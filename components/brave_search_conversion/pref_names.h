/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_

class PrefService;

namespace brave_search_conversion {
namespace prefs {

constexpr char kDismissed[] = "brave.brave_search_conversion.dismissed";

constexpr char kP3ABannerShown[] = "brave.brave_search_conversion.banner_shown";
constexpr char kP3AButtonShown[] = "brave.brave_search_conversion.button_shown";
constexpr char kP3ANTPShown[] = "brave.brave_search_conversion.ntp_shown";

constexpr char kP3ABannerTriggered[] =
    "brave.brave_search_conversion.banner_triggered";
constexpr char kP3AButtonTriggered[] =
    "brave.brave_search_conversion.button_triggered";
constexpr char kP3ANTPTriggered[] =
    "brave.brave_search_conversion.ntp_triggered";

constexpr char kP3ADefaultEngineChanged[] =
    "brave.brave_search_conversion.default_changed";

}  // namespace prefs
}  // namespace brave_search_conversion

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_PREF_NAMES_H_
