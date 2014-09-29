/*
 * plugin.c
 * Copyright 2010-2012 Michał Lipski
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include "oss.h"

#include <libaudcore/audstrings.h>
#include <libaudcore/preferences.h>

static Index<ComboItem> oss_elements;

static void combo_init()
{
    int mixerfd;

    CHECK_NOISY(mixerfd = open, DEFAULT_MIXER, O_RDWR);

    oss_elements.append(ComboItem(strdup(N_("Default device")), strdup(DEFAULT_DSP)));

#ifdef SNDCTL_SYSINFO
    oss_sysinfo sysinfo;
    memset(&sysinfo, 0, sizeof sysinfo);
    CHECK(ioctl, mixerfd, SNDCTL_SYSINFO, &sysinfo);
    CHECK_NOISY(oss_probe_for_adev, &sysinfo);

    for (int i = 0; i < sysinfo.numaudios; i++)
    {
        oss_audioinfo ainfo;
        memset(&ainfo, 0, sizeof ainfo);
        ainfo.dev = i;

        CHECK(ioctl, mixerfd, SNDCTL_AUDIOINFO, &ainfo);

        if (ainfo.caps & PCM_CAP_OUTPUT)
            oss_elements.append(ComboItem(strdup(ainfo.name), strdup(ainfo.devnode)));
    }
#endif

FAILED:
    close(mixerfd);
}

ArrayRef<const ComboItem> combo_fill()
{
    return {oss_elements.begin(), oss_elements.len()};
}

static void combo_cleanup()
{
    for (ComboItem & elem : oss_elements)
    {
        free((char *)elem.label);
        free((char *)elem.str);
    }

    oss_elements.clear();
}

static const PreferencesWidget oss_widgets[] = {
    WidgetCombo(N_("Audio device:"),
        WidgetString ("oss4", "device"),
        {0, combo_fill}),
    WidgetCheck(N_("Use alternate device:"),
        WidgetBool ("oss4", "use_alt_device")),
    WidgetEntry(0, WidgetString ("oss4", "alt_device"),
        {}, WIDGET_CHILD),
    WidgetCheck(N_("Save volume between sessions."),
        WidgetBool ("oss4", "save_volume")),
    WidgetCheck(N_("Enable format conversions made by the OSS software."),
        WidgetBool ("oss4", "cookedmode")),
    WidgetCheck(N_("Enable exclusive mode to prevent virtual mixing."),
        WidgetBool ("oss4", "exclusive"))
};

static const PluginPreferences oss_prefs = {
    {oss_widgets},
    combo_init,
    nullptr,  // apply
    combo_cleanup
};

static const char oss_about[] =
 N_("OSS4 Output Plugin for Audacious\n"
    "Copyright 2010-2012 Michał Lipski\n\n"
    "I would like to thank people on #audacious, especially Tony Vroon and "
    "John Lindgren and of course the authors of the previous OSS plugin.");

// OSS4 is preferred over ALSA (priority 5).
// ALSA is preferred over OSS3.
#ifdef SNDCTL_SYSINFO
#define AUD_PLUGIN_NAME        N_("OSS4 Output")
#define AUD_OUTPUT_PRIORITY    6
#else
#define AUD_PLUGIN_NAME        N_("OSS3 Output")
#define AUD_OUTPUT_PRIORITY    4
#endif

#define AUD_PLUGIN_INIT        oss_init
#define AUD_PLUGIN_CLEANUP     oss_cleanup
#define AUD_OUTPUT_OPEN        oss_open_audio
#define AUD_OUTPUT_CLOSE       oss_close_audio
#define AUD_OUTPUT_WRITE       oss_write_audio
#define AUD_OUTPUT_DRAIN       oss_drain
#define AUD_OUTPUT_GET_FREE    oss_buffer_free
#define AUD_OUTPUT_WAIT_FREE   oss_period_wait
#define AUD_OUTPUT_GET_TIME    oss_output_time
#define AUD_OUTPUT_FLUSH       oss_flush
#define AUD_OUTPUT_PAUSE       oss_pause
#define AUD_OUTPUT_SET_VOLUME  oss_set_volume
#define AUD_OUTPUT_GET_VOLUME  oss_get_volume
#define AUD_PLUGIN_ABOUT       oss_about
#define AUD_PLUGIN_PREFS       &oss_prefs

#define AUD_DECLARE_OUTPUT
#include <libaudcore/plugin-declare.h>
