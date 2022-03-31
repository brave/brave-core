/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/default_protocol_handler_utils_win.h"

#include "base/strings/string_piece.h"
#include "testing/gtest/include/gtest/gtest.h"

// Below test cases are copied from
// https://github.com/mozilla/gecko-dev/blob/master/toolkit/mozapps/defaultagent/tests/gtest/SetDefaultBrowserTest.cpp

using protocol_handler_utils::GenerateUserChoiceHash;

TEST(DefaultProtocolHandlerUtilsWinTest, HashTest) {
  // Hashes set by System Settings on 64-bit Windows 10 Pro 20H2 (19042.928).
  const wchar_t* sid = L"S-1-5-21-636376821-3290315252-1794850287-1001";

  // length mod 8 = 0
  EXPECT_EQ(
      GenerateUserChoiceHash(L"https", sid, L"FirefoxURL-308046B0AF4A39CB",
                             (SYSTEMTIME){2021, 4, 1, 19, 23, 7, 56, 506}),
      L"uzpIsMVyZ1g=");

  // length mod 8 = 2 (confirm that the incomplete last block is dropped)
  EXPECT_EQ(
      GenerateUserChoiceHash(L".html", sid, L"FirefoxHTML-308046B0AF4A39CB",
                             (SYSTEMTIME){2021, 4, 1, 19, 23, 7, 56, 519}),
      L"7fjRtUPASlc=");

  // length mod 8 = 4
  EXPECT_EQ(
      GenerateUserChoiceHash(L"https", sid, L"MSEdgeHTM",
                             (SYSTEMTIME){2021, 4, 1, 19, 23, 3, 48, 119}),
      L"Fz0kA3Ymmps=");

  // length mod 8 = 6
  EXPECT_EQ(GenerateUserChoiceHash(L".html", sid, L"ChromeHTML",
                                   (SYSTEMTIME){2021, 4, 1, 19, 23, 6, 3, 628}),
            L"R5TD9LGJ5Xw=");

  // non-ASCII
  EXPECT_EQ(
      GenerateUserChoiceHash(L".html", sid, L"FirefoxHTML-ÀBÇDË😀†",
                             (SYSTEMTIME){2021, 4, 2, 20, 0, 38, 55, 101}),
      L"F3NsK3uNv5E=");
}
