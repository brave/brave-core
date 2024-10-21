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

// Matchers are a set of conditions using AND logic that must be met for an ad
// to be served.
//
// pref_path:
//
// Supports booleans, integers, doubles, strings, nested dictionaries, nested
// lists, and dot-separated path keys. `base::Value::Find*ByDottedPath` is not
// used because path keys can contain dots. Returns `std::nullopt` if the path
// was not found in either profile or local state preferences. Path keys should
// be separated by `|`, lists should be followed by an index, i.e., `list|1`,
// and dictionaries should be followed by a key, i.e., `dict|key`.
//
// condition:
//
// [operator]:days matcher: The operator can be '=', '>', or '≥'. '=' is used
// for exact matches, '>' for greater than, and '≥' for greater than or equal
// to. This is used to serve an ad if an event occurred a specified number of
// `days` ago or more from the timestamp, either a Unix epoch or a Windows epoch
// timestamp stored at `pref_path`. For example, `pref_path=[=]:3` will serve an
// ad if the event occurred exactly 3 days ago.
//
// regex matcher: The regex string is a regular expression that must be matched
// by the value at `pref_path`. For example, `pref_path=(0|1|3)` will serve an
// ad if the value at `pref_path` contains 0, 1, or 3. Uses Google's secure
// regular expression engine, RE2.
//
// pattern matcher: The pattern string can contain * and ? wildcards. The
// backslash \ character is an escape character for * and ?. ? matches 0 or 1
// character, while * matches 0 or more characters. For example,
// `brave.brave_ads.enabled=0` will serve an ad if the value stored at
// `pref_path` equals `0`, i.e., false.

bool MatchConditions(const PrefProviderInterface* pref_provider,
                     const NewTabPageAdConditionMatchers& condition_matchers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_H_
