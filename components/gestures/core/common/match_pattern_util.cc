// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/match_pattern_util.h"

#include "base/strings/pattern.h"
#include "third_party/re2/src/re2/re2.h"

namespace gestures {

bool HasRegexOnlySyntax(std::string_view pattern) {
  return pattern.find_first_of("^$|(){}[]\\") != std::string_view::npos;
}

bool MatchRegex(std::string_view text, std::string_view pattern) {
  const re2::RE2 regex(pattern, re2::RE2::Quiet);
  if (!regex.ok()) {
    return false;
  }
  return re2::RE2::PartialMatch(text, regex);
}

bool MatchesPattern(std::string_view text, std::string_view pattern) {
  return HasRegexOnlySyntax(pattern) ? MatchRegex(text, pattern)
                                     : base::MatchPattern(text, pattern);
}

}  // namespace gestures
