/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_PREF_NAMES_H_

namespace speedreader {

// Deprecated - migrated to kSpeedreaderPrefFeatureEnabled and
// kSpeedreaderAllowedForAllReadableSites (08/2025)
// java_cpp_string.py doesn't work when the variable is constexpr
inline constexpr char kSpeedreaderPrefEnabledDeprecated[] =
    "brave.speedreader.enabled";

// Set if Speedreader was enabled at least once
inline constexpr char kSpeedreaderPrefEverEnabled[] =
    "brave.speedreader.ever_enabled";

// Number of times the user has toggled Speedreader
inline constexpr char kSpeedreaderPrefToggleCount[] =
    "brave.speedreader.toggle_count";

// Number of times the "Enable Speedreader" button was shown automatically
inline constexpr char kSpeedreaderPrefPromptCount[] =
    "brave.speedreader.prompt_count";

// The theme selected by the user. If it has a default value then system theme
// should be used.
inline constexpr char kSpeedreaderPrefTheme[] = "brave.speedreader.theme";

inline constexpr char kSpeedreaderPrefFontSize[] =
    "brave.speedreader.font_size";

inline constexpr char kSpeedreaderPrefFontFamily[] =
    "brave.speedreader.font_family";

inline constexpr char kSpeedreaderPrefColumnWidth[] =
    "brave.speedreader.column_width";

inline constexpr char kSpeedreaderPrefTtsVoice[] =
    "brave.speedreader.tts_voice";

inline constexpr char kSpeedreaderPrefTtsSpeed[] =
    "brave.speedreader.tts_speed";

inline constexpr char kSpeedreaderPageViewsStoragePref[] =
    "brave.speedreader.page_views";

// Top-level feature toggle for Speedreader (named feature_enabled to
// distinguish from the legacy "enabled" pref which actually controls all-sites
// behavior)
inline constexpr char kSpeedreaderEnabled[] =
    "brave.speedreader.feature_enabled";

// Whether Speedreader should be enabled for all sites
inline constexpr char kSpeedreaderAllowedForAllReadableSites[] =
    "brave.speedreader.enabled_for_all_sites";

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_PREF_NAMES_H_
