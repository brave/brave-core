/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_test_whitelist.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/strings/pattern.h"
#include "base/strings/string_split.h"
#include "brave/components/speedreader/speedreader_switches.h"
#include "url/gurl.h"

namespace speedreader {

namespace {

std::vector<std::string> GetHardcodedWhitelist() {
  static const base::NoDestructor<std::vector<std::string>> whitelist({
    "https://medium.com/*/*",
    "https://longreads.com/*/*",
    "https://edition.cnn.com/*",
  });
  return *whitelist;
}

}  // namespace

bool IsWhitelistedForTest(const GURL& url) {
  const auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (!cmd_line->HasSwitch(kSpeedreaderWhitelist)) {
    // Nothing whitelisted.
    return false;
  }
  const std::string whitelist_str =
      cmd_line->GetSwitchValueASCII(kSpeedreaderWhitelist);
  auto whitelist = base::SplitString(whitelist_str,
                                     ";",
                                     base::WhitespaceHandling::TRIM_WHITESPACE,
                                     base::SplitResult::SPLIT_WANT_NONEMPTY);
  const auto& hardcoded = GetHardcodedWhitelist();
  whitelist.insert(whitelist.end(), hardcoded.begin(), hardcoded.end());

  for (const auto& pattern : whitelist) {
    if (base::MatchPattern(url.spec(), pattern)) {
      return true;
    }
  }
  return false;
}

}  // namespace speedreader
