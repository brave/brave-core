/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/history/history_utils.h"
#include "components/previews/core/previews_experiments.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "extensions/buildflags/buildflags.h"

// This test covers all cases that upstream and our version of
// CanAddURLToHistory().
TEST(HistoryUtilsTest, VariousURLTest) {
  EXPECT_TRUE(CanAddURLToHistory(GURL("https://www.brave.com/")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("brave://sync/")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("javascript://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("about://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("content://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("devtools://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("chrome://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("view-source://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("chrome-native://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("chrome-search://test")));
  EXPECT_FALSE(CanAddURLToHistory(GURL("chrome-distiller://test")));
#if BUILDFLAG(ENABLE_EXTENSIONS)
  EXPECT_FALSE(CanAddURLToHistory(
      GURL("chrome-extension://odbfpeeihdkbihmopkbjmoonfanlbfcl/home.html")));
#endif
}
