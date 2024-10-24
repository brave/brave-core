/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_H_

#include <map>
#include <string>

namespace brave_ads {

using NewTabPageAdConditionMatchers =
    std::multimap</*pref_path*/ std::string, /*condition*/ std::string>;

class PrefProviderInterface;

// Matchers are one or more preference paths and conditions, using AND logic,
// that must all be met for a new tab takeover ad to be served. Preference path
// keys should be separated by |, where paths may include list indices (e.g.,
// "list|1") or dictionary keys (e.g., "dict|key"). Paths can also be nested.
// Both Brave local state and profile preferences are supported. See
// https://github.com/brave/brave-browser/wiki/P3A for more information on P3A
// pref path usage.
//
// For non-Rewards users, condition matchers should be included in the
// "photo.json" file under the NTP (New Tab Page) sponsored images component,
// within "campaigns2", falling back to "campaigns", or falling back to the root
// "campaign" for backwards compatibility.
//
// For Rewards users, these matchers should be placed in the catalog under
// "wallpapers" with the "imageUrl" prefixed with "[SmartNTT]" for backwards
// compatibility, where legacy browsers will discard these wallpapers due to an
// invalid URL.
//
// If no condition matchers are present ads will always be shown, unless
// frequency capped for Rewards users. If condition matchers are malformed or
// have unknown pref paths, the ad will not be shown.
//
// Supported Condition Matchers:
//
// 1. [operator]:days Matcher:
//    - Support operators:
//      - '=': Exact
//      - '>': Greater than
//      - '≥': Greater than or equal to
//      - '<': Less than
//      - '≤': Less than or equal to
//    - This matcher triggers an ad based on when an event occurred or will
//      occur, using a timestamp (Unix or Windows epoch) stored at "prefPath".
//      For instance, the example below will serve an ad only if the timestamp
//      for "foo|bar" occurred more than 3 days ago:
//
//       "conditionMatchers": [
//         {
//           "condition": "[>]:3",
//           "prefPath": "foo|bar"
//         }
//       ]
//
// 2. Regex Matcher:
//    - Uses an RE2 regular expression to partially match values at "prefPath",
//      see https://github.com/google/re2/wiki/syntax. For example, the
//      following will serve an ad if the value at "foo|bar" starts with "abc":
//
//       "conditionMatchers": [
//         {
//            "condition": "^abc",
//            "prefPath": "foo|bar"
//         }
//       ]
//
// 3. Pattern Matcher:
//    - Supports wildcards "*" and "?". "*" matches zero or more characters,
//      while "?" matches zero or one character. To use these literally, escape
//      them with "\". In the example below, an ad will be served only if the
//      value at "foo|bar" matches the pattern "*baz?qux*":
//
//       "conditionMatchers": [
//         {
//           "condition": "*baz?qux*",
//            "prefPath": "foo|bar"
//         }
//       ]
//
// For example, the following condition matchers would only serve a new tab
// takeover ad if the default search provider is set to "Startpage", the user
// has exactly one bookmark, and the browser was installed between three and
// seven days ago:
//
//  "conditionMatchers": [
//    {
//      "condition": "*-538868000510",
//      "prefPath": "default_search_provider|guid"
//    },
//    {
//      "condition": "1",
//      "prefPath": "p3a|logs_constellation_prep|Brave.Core.BookmarkCount|value"
//    },
//    {
//      "condition": "[≥]:3",
//      "prefPath": "uninstall_metrics|installation_date2"
//    },
//    {
//      "condition": "[≤]:7",
//      "prefPath": "uninstall_metrics|installation_date2"
//    }
//  ]
//
// We support virtual preferences for values that are not stored in the profile
// or local state preferences. Virtual preference path keys should be prefixed
// with "[virtual]:".
//
// "[virtual]:build_channel.name" retrieves the build channel of the browser,
// returning one of the following values: "stable", "beta", "dev", "nightly", or
// "unknown".
//
// "[virtual]:default_search_engine.name" retrieves the default search engine
// chosen during browser installation, returning one of the following values:
// "Brave", "Google", "Yandex", "Bing", "Daum", "네이버", "DuckDuckGo", "Qwant",
// "Startpage", or "Ecosia".
//
// NOTE: To identify condition matchers, first create a copy of your preference
// files. Next, change a brave://setting or enable a feature, quit the browser
// and then compare the original and modified versions to determine which
// key/value pairs are required. Invalid or malformed condition matchers will be
// logged to the console, they are not logged to the Rewards log.

bool MatchConditions(const PrefProviderInterface* pref_provider,
                     const NewTabPageAdConditionMatchers& condition_matchers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_H_
