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

// Matchers are one or more pref paths and conditions, using AND logic, that
// must all be met for an ad to be served. Pref path keys should be separated by
// "|", where paths may include list indices (e.g., "list|1") or dictionary keys
// (e.g., "dict|key"). Paths can also be nested. Both Brave local state and
// profile prefs are supported.
//
// For non-Rewards users, condition matchers should be included in the
// "photo.json" file under the NTP (New Tab Page) sponsored images component,
// within "campaigns2", falling back to "campaigns", or the root "campaign" for
// backwards compatibility.
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
// 1. [epoch operator]:days Matcher:
//    - Support operators:
//      - 'T=': Equal
//      - 'T>': Greater than
//      - 'T≥': Greater than or equal to
//      - 'T<': Less than
//      - 'T≤': Less than or equal to
//    - This matcher triggers an ad based on when an event occurred or will
//      occur, using a timestamp (Unix or Windows epoch) stored at "prefPath".
//      For instance, the example below will serve an ad only if the timestamp
//      for "foo.bar" occurred more than 3 days ago:
//
//       "conditionMatchers": [
//         {
//           "condition": "[T>]:3",
//           "prefPath": "foo.bar"
//         }
//       ]
//
// 2. [numerical operator]:number Matcher:
//    - Support operators:
//      - 'R=': Equal
//      - 'R≠': Not equal
//      - 'R>': Greater than
//      - 'R≥': Greater than or equal to
//      - 'R<': Less than
//      - 'R≤': Less than or equal to
//    - This matcher triggers an ad based on when a real number (integers or
//      fractional) stored at "prefPath". For instance, the example below will
//      serve an ad only if the value stored at "foo.bar" is not equal to 3:
//
//       "conditionMatchers": [
//         {
//           "condition": "[R≠]:3",
//           "prefPath": "foo.bar"
//         }
//       ]
//
// 3. Regex Matcher:
//    - Uses an RE2 regular expression to partially match values at "prefPath",
//      see https://github.com/google/re2/wiki/syntax. For example, the
//      following will serve an ad if the value at "foo.bar" starts with "abc":
//
//       "conditionMatchers": [
//         {
//            "condition": "^abc",
//            "prefPath": "foo.bar"
//         }
//       ]
//
// 4. Pattern Matcher:
//    - Supports wildcards "*" and "?". "*" matches zero or more characters,
//      while "?" matches zero or one character. To use these literally, escape
//      them with "\". In the example below, an ad will be served only if the
//      value at "foo.bar" matches the pattern "*baz?qux*":
//
//       "conditionMatchers": [
//         {
//           "condition": "*baz?qux*",
//            "prefPath": "foo.bar"
//         }
//       ]
//
// For example, the following condition matchers would only serve a new tab
// takeover ad if the default search provider is set to "Startpage", the user
// has less than 10 bookmarks, and the browser was installed between three and
// seven days ago:
//
//  "conditionMatchers": [
//    {
//      "condition": "*-538868000510",
//      "prefPath": "default_search_provider.guid"
//    },
//    {
//      "condition": "[R<]:10",
//      "prefPath": "p3a.logs_constellation_prep|Brave.Core.BookmarkCount|value"
//    },
//    {
//      "condition": "[T≥]:3",
//      "prefPath": "uninstall_metrics.installation_date2"
//    },
//    {
//      "condition": "[T≤]:7",
//      "prefPath": "uninstall_metrics.installation_date2"
//    }
//  ]
//
// We support virtual prefs for values that are not stored in the profile or
// local state prefs. Virtual pref path keys should be prefixed with
// "[virtual]:".
//
// "[virtual]:browser|version" retrieves the browser version, e.g. "72.0.59.3".
//
// "[virtual]:browser|build_channel" retrieves the build channel of the browser,
// returning one of the following values: "stable", "beta", "dev", "nightly", or
// "unknown".
//
// "[virtual]:operating_system|locale|language" retrieves the operating system's
// language, e.g., "en", and "[virtual]:operating_system|locale|region"
// retrieves the operating system's region, e.g., "US".
//
// "[virtual]:operating_system|name" retrieves the operating system, returning
// one of the following values: "Windows", "Mac OS X", "Linux", "Android",
// "iOS", or "Unknown".
//
// "[virtual]:search_engine|default_name" retrieves the default search engine
// chosen during browser installation, returning one of the following values:
// "Brave", "Google", "Yandex", "Bing", "Daum", "네이버", "DuckDuckGo", "Qwant",
// "Startpage", or "Ecosia". For the most up-to-date list of possible default
// search engines, see `TemplateURLPrepopulateData::GetDefaultSearchEngine`.
//
// "[virtual]:skus|environment|location|key" retrieves the value from either the
// production or staging environment, the "talk.brave.com", "vpn.brave.com", or
// "leo.brave.com" location, and the "created_at", "expires_at", "last_paid_at",
// or "status" key. Status returns one of the following values: `trial`, `beta`,
// `paid`, or `canceled`. For example, the following will serve an ad if the
// user has canceled their Brave VPN subscription:
//
//  "conditionMatchers": [
//    {
//       "condition": "canceled",
//       "prefPath": "[virtual]:skus|production|vpn.brave.com|status"
//    }
//  ]
//
// NOTE: To identify condition matchers, first create a copy of your pref files.
// Next, change a brave://setting or enable a feature, quit the browser and then
// compare the original and modified versions to determine which key/value pairs
// are required. Invalid or malformed condition matchers will be logged to the
// console, they are not logged to the Rewards log.

bool MatchConditions(const PrefProviderInterface* pref_provider,
                     const ConditionMatcherMap& condition_matchers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_TARGETING_CONDITION_MATCHER_CONDITION_MATCHER_UTIL_H_
