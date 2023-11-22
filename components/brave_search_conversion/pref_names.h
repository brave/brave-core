/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_

class PrefService;

namespace brave_search_conversion {
namespace prefs {

inline constexpr char kDismissed[] = "brave.brave_search_conversion.dismissed";
inline constexpr char kMaybeLaterClickedTime[] =
    "brave.brave_search_conversion.maybe_later_clicked_time";

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

}  // namespace prefs
}  // namespace brave_search_conversion

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_
