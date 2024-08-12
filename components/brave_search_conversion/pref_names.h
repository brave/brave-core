/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_

class PrefService;

namespace brave_search_conversion::prefs {

inline constexpr char kDismissed[] = "brave.brave_search_conversion.dismissed";
inline constexpr char kMaybeLaterClickedTime[] =
    "brave.brave_search_conversion.maybe_later_clicked_time";

// Index ranges from 0 to 1 and it's matched DDGTypeC to DDGTypeD.
// It's the type index for now.
inline constexpr char kDDGBannerTypeIndex[] =
    "brave.brave_search_conversion.ddg_banner_type_index";

// It's the time that current index type is shown first.
// We rotate 4 types when specific time is passed.
inline constexpr char kLatestDDGBannerTypeFirstShownTime[] =
    "brave.brave_search_conversion.latest_ddg_banner_type_first_shown_time";

inline constexpr char kShowNTPSearchBox[] =
    "brave.brave_search.show-ntp-search";

// Determines whether the search box on the NTP prompts the user to enable
// search suggestions.
inline constexpr char kPromptEnableSuggestions[] =
    "brave.brave_search.ntp-search_prompt_enable_suggestions";

inline constexpr char kP3AActionStatuses[] =
    "brave.brave_search_conversion.action_statuses";

inline constexpr char kP3ADefaultEngineConverted[] =
    "brave.brave_search_conversion.default_changed";
const char kP3AQueryCountBeforeChurn[] =
    "brave.brave_search_conversion.query_count";
const char kP3AAlreadyChurned[] =
    "brave.brave_search_conversion.already_churned";

inline constexpr char kP3ABannerShown[] =
    "brave.brave_search_conversion.banner_shown";  // DEPRECATED
inline constexpr char kP3ABannerTriggered[] =
    "brave.brave_search_conversion.banner_triggered";  // DEPRECATED
inline constexpr char kP3AButtonShown[] =
    "brave.brave_search_conversion.button_shown";  // DEPRECATED
inline constexpr char kP3ANTPShown[] =
    "brave.brave_search_conversion.ntp_shown";  // DEPRECATED
inline constexpr char kP3AButtonTriggered[] =
    "brave.brave_search_conversion.button_triggered";  // DEPRECATED
inline constexpr char kP3ANTPTriggered[] =
    "brave.brave_search_conversion.ntp_triggered";  // DEPRECATED

}  // namespace brave_search_conversion::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_
