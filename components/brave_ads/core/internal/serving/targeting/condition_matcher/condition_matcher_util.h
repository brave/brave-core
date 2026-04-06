/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_

#include <map>
#include <string>

namespace base {
class DictValue;
}  // namespace base

// Condition matchers must undergo a privacy review.
//
// Matchers are one or more pref paths and conditions, using AND logic, that
// must all match before an ad is eligible to be served. Path components should
// be separated by "|", with the exception that registered pref paths must use a
// dotted path. Paths may include list indices (e.g., "list|1") or dictionary
// keys (e.g., "dict|key"). Paths can be nested. Both local state and profile
// prefs are supported. This process only occurs on the device and never leaves
// the device.
//
// If no condition matchers are present ads will always be shown, unless
// frequency capped. If condition matchers are malformed or have unknown pref
// paths, the ad will not be shown. Epoch operators use the current time when
// the pref is absent; numerical operators use 0.
//
// Supported Condition Matchers:
//
// 1. [pref path operator] Matcher:
//    - Supported operators:
//      - [!]: Does not exist
//    - This matcher matches if a "prefPath" does not exist. For example, the
//      following condition matcher will match if the pref path "foo.bar" does
//      not exist:
//
//  "conditionMatchers": [
//    {
//      "prefPath": "[!]:foo.bar"
//    }
//  ]
//
// 2. [epoch operator]:days Matcher:
//    - Supported operators:
//      - [T=]: Equal
//      - [T≠]: Not equal
//      - [T>]: Greater than
//      - [T≥]: Greater than or equal to
//      - [T<]: Less than
//      - [T≤]: Less than or equal to
//    - This matcher matches based on when an event occurred or will occur,
//      using a timestamp (Unix, Windows, or ISO 8601 format) stored at
//      "prefPath". If the pref is absent, the current time is used as the
//      default. For example, the following condition matcher will match if the
//      timestamp for "foo.bar" occurred more than 3 days ago:
//
//       "conditionMatchers": [
//         {
//           "condition": "[T>]:3",
//           "prefPath": "foo.bar"
//         }
//       ]
//
// 3. [numerical operator]:number Matcher:
//    - Supported operators:
//      - [R=]: Equal
//      - [R≠]: Not equal
//      - [R>]: Greater than
//      - [R≥]: Greater than or equal to
//      - [R<]: Less than
//      - [R≤]: Less than or equal to
//    - This matcher matches based on the real number (integers or fractional)
//      stored at "prefPath". If the pref is absent, 0 is used as the default.
//      The operand can be a literal number or a pref path whose value is
//      resolved at match time. For example, the following condition matcher
//      will match if the value stored at "foo.bar" is not equal to 3:
//
//       "conditionMatchers": [
//         {
//           "condition": "[R≠]:3",
//           "prefPath": "foo.bar"
//         }
//       ]
//
//      Or, to compare "foo.bar" against the numeric value stored at "baz.qux":
//
//       "conditionMatchers": [
//         {
//           "condition": "[R>]:baz.qux",
//           "prefPath": "foo.bar"
//         }
//       ]
//
//      If the operand is not a literal number and cannot be resolved as a pref
//      path, or the resolved pref value is non-numeric, the condition will not
//      match.
//
// 4. Regex Matcher:
//    - Uses an RE2 regular expression to partially match values at "prefPath",
//      see https://github.com/google/re2/wiki/syntax. For example, the
//      following condition matcher will match if the value at "foo.bar" is
//      either "fred" or "waldo":
//
//       "conditionMatchers": [
//         {
//            "condition": "^(fred|waldo)$",
//            "prefPath": "foo.bar"
//         }
//       ]
//
// 5. Pattern Matcher:
//    - Supports wildcards "*" and "?". "*" matches zero or more characters,
//      while "?" matches zero or one character. To use these literally, escape
//      them with "\". For example, the following condition matcher will match
//      if the value at "foo.bar" matches the pattern "*baz?qux*":
//
//       "conditionMatchers": [
//         {
//           "condition": "*baz?qux*",
//           "prefPath": "foo.bar"
//         }
//       ]
//
// 6. Time Period Storage Matcher:
//    - When a path component refers to a list-valued pref, this path component
//      sums the "value" fields of entries whose "day" timestamp falls within
//      the given duration, returning a double. Omit the duration or use "all"
//      to sum all entries. Valid duration units are "d" (days), "h" (hours),
//      "m" (minutes), "s" (seconds). For example, the following condition
//      matcher will match if the sum of "list" entries in the last 7 days is
//      less than 5:
//
//       "conditionMatchers": [
//         {
//           "condition": "[R<]:5",
//           "prefPath": "list|time_period_storage=7d"
//         }
//       ]
//
// For example, the following condition matcher will match if the
// `brave.new_tab_page.show_brave_news` pref path does not exist, the browser
// was installed 3 or more days ago, the user has fewer than 7 bookmarks, the
// chosen search provider is Startpage, and both the brave://settings/languages
// preferred website language and the operating system language are Deutsch:
//
//  "conditionMatchers": [
//    {
//      "prefPath": "[!]:brave.new_tab_page.show_brave_news"
//    },
//    {
//      "condition": "[T≥]:3",
//      "prefPath": "uninstall_metrics.installation_date2"
//    },
//    {
//      "condition": "[R<]:7",
//      "prefPath": "p3a.logs_constellation_prep|Brave.Core.BookmarkCount|value"
//    },
//    {
//      "condition": "de$",
//      "prefPath": "intl.selected_languages"
//    },
//    {
//      "condition": "*-538868000510",
//      "prefPath": "default_search_provider.guid"
//    },
//    {
//      "condition": "de",
//      "prefPath": "[virtual]:operating_system|locale|language"
//    }
//  ]
//
// Pref paths are resolved by checking virtual prefs first then profile prefs
// then local state prefs. Virtual prefs are computed at runtime and not stored.
// Prefix virtual pref paths with "[virtual]".
//
//  Supported virtual pref paths:
//
//  1. "[virtual]:browser|version"
//     - Returning the browser version string, e.g. "1.91.0". Use a regex
//       condition to match on this value.
//
//  2. "[virtual]:browser|build_channel"
//     - Returning the build channel of the browser, returning one of the
//       following values: "stable", "beta", "dev", "nightly", or "unknown".
//
//  3. "[virtual]:operating_system|locale|language"
//     - Returning the operating system's language, e.g., "en".
//
//  4. "[virtual]:operating_system|locale|region"
//     - Returning the operating system's region, e.g., "US".
//
//  5. "[virtual]:operating_system|name"
//     - Returning the operating system, returning one of the following values:
//       "Windows", "Mac OS X", "Linux", "Android", "iOS", or "Unknown".
//
//  6. "[virtual]:search_engine|default_name"
//     - Returning the default search engine chosen during browser installation,
//       returning one of the following values: "Brave", "Google", "Yandex",
//       "Bing", "Daum", "네이버", "DuckDuckGo", "Qwant", "Startpage", or
//       "Ecosia". For the most up-to-date list of possible default search
//       engines, see `TemplateURLPrepopulateData::GetDefaultSearchEngine`.
//
//  7. "[virtual]:skus|environment|location|key"
//     - Returning the SKU value from either the production or staging
//       environment, for the given location, i.e., "vpn.brave.com",
//       "search.brave.com", "leo.brave.com", or "talk.brave.com", and the
//       "created_at", "expires_at", "last_paid_at", or "status" key. Status
//       returns one of the following values: `trial`, `beta`, `paid`, or
//       `canceled`. For example, the following condition matcher will match if
//       the user has canceled their Brave VPN subscription:
//
//        "conditionMatchers": [
//          {
//             "condition": "canceled",
//             "prefPath": "[virtual]:skus|production|vpn.brave.com|status"
//          }
//        ]
//
//  8. "[virtual]:serp_metrics|search_engine|time_period_storage[=<duration>]"
//     - Returning the sum of SERP visits recorded for the given search engine
//       over the specified time window. "search_engine" must be one of
//       "brave_search_engine", "google_search_engine", or
//       "other_search_engine". The optional duration (e.g. "7d", "1h", "30m")
//       sums entries within that window, or "all" (or omitted) to sum the
//       entire history. Valid duration units are "d" (days), "h" (hours), "m"
//       (minutes), "s" (seconds). For example, the following condition matcher
//       compares SERP visit counts across two search engines over the last 7
//       days:
//
//        "conditionMatchers": [
//          {
//            "condition":
//            "[R>]:[virtual]:serp_metrics|brave_search_engine|time_period_storage=7d",
//            "prefPath":
//            "[virtual]:serp_metrics|google_search_engine|time_period_storage=7d"
//          }
//        ]
//
//  NOTE: To identify condition matchers, first create a copy of your
//  brave://local-state and `Default/Preferences` files. Next, change a
//  brave://setting or enable a feature, quit the browser and then compare the
//  original and modified versions to determine which key/value pairs are
//  required. Invalid or malformed condition matchers will be logged to the
//  console, they are not logged to the Rewards log.

namespace brave_ads {

using ConditionMatcherMap =
    std::multimap</*pref_path*/ std::string, /*condition*/ std::string>;

// Returns `true` if all conditions in `condition_matchers` match their
// respective pref values, or `false` if any condition fails or a pref path is
// malformed or unknown.
bool MatchConditions(const base::DictValue& virtual_prefs,
                     const ConditionMatcherMap& condition_matchers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_
