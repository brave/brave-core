// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_MATCH_PATTERN_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_MATCH_PATTERN_UTIL_H_

#include <string_view>

namespace gestures {

// Whether `pattern` uses syntax that only means something in regex, e.g.
// "^$|(){}[]\"; determines whether `MatchesPattern` tries regex over glob.
bool HasRegexOnlySyntax(std::string_view pattern);

// Returns `true` if `pattern` (an RE2 expression) partially matches `text`,
// or `false` if it is not valid.
bool MatchRegex(std::string_view text, std::string_view pattern);

// Matches `text` against `pattern` as a regex if it uses regex-only syntax,
// falling back to a plain glob (`base::MatchPattern`) otherwise.
bool MatchesPattern(std::string_view text, std::string_view pattern);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_MATCH_PATTERN_UTIL_H_
