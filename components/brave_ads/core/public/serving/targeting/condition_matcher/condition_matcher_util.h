/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_

#include <map>
#include <string>

namespace brave_ads {

using ConditionMatcherMap =
    std::multimap</*pref_path*/ std::string, /*condition*/ std::string>;

class PrefProviderInterface;

// Condition matchers must undergo a privacy review.
//
// Matchers are one or more pref paths and conditions, using AND logic, that
// must all be met for an ad to be served. Pref path keys should be separated by
// "|", with the exception that registered pref paths must use a dotted path.
// Paths may include list indices (e.g., "list|1") or dictionary keys (e.g.,
// "dict|key"). Paths can be nested. Both local state and profile prefs are
// supported. This process only occurs on the device and never leaves the
// device.
//
// If no condition matchers are present ads will always be shown, unless
// frequency capped for Rewards users. If condition matchers are malformed or
// have unknown pref paths, the ad will not be shown.
//
// Supported Condition Matchers:
//
// 1. [pref path operator] Matcher:
//    - Supported operators:
//      - [!]: Not equal
//    - This matcher serves an ad if a "prefPath" does not exist. For example,
//      the following condition matcher will serve an ad if the pref path
//      "foo.bar" does not exist:
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
//    - This matcher serves an ad based on when an event occurred or will occur,
//      using a timestamp (Unix, Windows, or ISO 8601 format) stored at
//      "prefPath". For example, the following condition matcher will serve an
//      ad if the timestamp for "foo.bar" occurred more than 3 days ago:
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
//    - This matcher serves an ad based on the real number (integers or
//      fractional) stored at "prefPath". For example, the following condition
//      matcher will serve an ad if the value stored at "foo.bar" is not equal
//      to 3:
//
//       "conditionMatchers": [
//         {
//           "condition": "[R≠]:3",
//           "prefPath": "foo.bar"
//         }
//       ]
//
// 4. Regex Matcher:
//    - Uses an RE2 regular expression to partially match values at "prefPath",
//      see https://github.com/google/re2/wiki/syntax. For example, the
//      following condition matcher will serve an ad if the value at "foo.bar"
//      is either "fred" or "waldo":
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
//      them with "\". For example, the following condition matcher will serve
//      an ad if the value at "foo.bar" matches the pattern "*baz?qux*":
//
//       "conditionMatchers": [
//         {
//           "condition": "*baz?qux*",
//           "prefPath": "foo.bar"
//         }
//       ]
//
// For example, the following condition matcher will serve an ad if the
// `brave.new_tab_page.show_brave_news` pref path does not exist, the browser
// was installed 3 or more days ago, the user has fewer than 7 bookmarks, the
// chosen search provider is Startpage, and both the brave://settings/languages
// preferred website language and the operating system language are Deutsch:
//
//  "conditionMatchers": [
//    {
//      "prefPath": "!brave.new_tab_page.show_brave_news"
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
// For values that are not stored in the local state or profile prefs you can
// use virtual prefs. Prefix virtual pref paths with "[virtual]:".
//
// Supported virtual pref paths:
//
// 1. "[virtual]:browser|version"
//    - Returning the browser version, e.g. "72.0.59.3".
//
// 2. "[virtual]:browser|build_channel"
//    - Returning the build channel of the browser, returning one of the
//      following values: "stable", "beta", "dev", "nightly", or "unknown".
//
// 3. "[virtual]:operating_system|locale|language"
//    - Returning the operating system's language, e.g., "en".
//
// 4. "[virtual]:operating_system|locale|region"
//    - Returning the operating system's region, e.g., "US".
//
// 5. "[virtual]:operating_system|name"
//    - Returning the operating system, returning one of the following values:
//      "Windows", "Mac OS X", "Linux", "Android", "iOS", or "Unknown".
//
// 6. "[virtual]:search_engine|default_name"
//    - Returning the default search engine chosen during browser installation,
//      returning one of the following values: "Brave", "Google", "Yandex",
//      "Bing", "Daum", "네이버", "DuckDuckGo", "Qwant", "Startpage", or
//      "Ecosia". For the most up-to-date list of possible default search
//      engines, see `TemplateURLPrepopulateData::GetDefaultSearchEngine`.
//
// 7. "[virtual]:skus|environment|location|key"
//    - Returning the SKU value from either the production or staging
//      environment, for the given location, i.e., "vpn.brave.com",
//      "search.brave.com", "leo.brave.com", or "talk.brave.com", and the
//      "created_at", "expires_at", "last_paid_at", or "status" key. Status
//      returns one of the following values: `trial`, `beta`, `paid`, or
//      `canceled`. For example, the following condition matcher will serve an
//      ad if the user has canceled their Brave VPN subscription:
//
//       "conditionMatchers": [
//         {
//            "condition": "canceled",
//            "prefPath": "[virtual]:skus|production|vpn.brave.com|status"
//         }
//       ]
//
// NOTE: To identify condition matchers, first create a copy of your `Local
// State` and `Default/Preferences` files. Next, change a brave://setting or
// enable a feature, quit the browser and then compare the original and modified
// versions to determine which key/value pairs are required. Invalid or
// malformed condition matchers will be logged to the console, they are not
// logged to the Rewards log.

bool MatchConditions(const PrefProviderInterface* pref_provider,
                     const ConditionMatcherMap& condition_matchers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_
