/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_COMMON_PREF_NAMES_H_

#include "base/component_export.h"
#include "build/build_config.h"

class PrefRegistrySimple;

namespace youtube_script_injector::prefs {

inline constexpr char kYouTubeBackgroundVideoPlaybackEnabled[] =
    "brave.youtube_script_injector.youtube_background_video_playback_enabled";

inline constexpr char kYouTubeExtraControlsEnabled[] =
    "brave.youtube_script_injector.youtube_extra_controls_enabled";

COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_COMMON)
void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace youtube_script_injector::prefs

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_COMMON_PREF_NAMES_H_
