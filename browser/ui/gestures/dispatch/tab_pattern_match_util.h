// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_PATTERN_MATCH_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_PATTERN_MATCH_UTIL_H_

#include <string_view>

namespace gestures {

// Returns `true` if `pattern` (an RE2 expression) partially matches `text`,
// or `false` if it is not valid.
bool MatchRegex(std::string_view text, std::string_view pattern);

// Matches `text` against `pattern`, trying it as a regex first if `pattern`
// uses syntax that only means something in regex, falling back to a plain
// glob (see `base::MatchPattern`) otherwise.
bool MatchesPattern(std::string_view text, std::string_view pattern);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_PATTERN_MATCH_UTIL_H_
