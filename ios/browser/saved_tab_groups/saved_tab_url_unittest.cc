// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/saved_tab_groups/saved_tab_url.h"

#include "components/saved_tab_groups/public/utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

namespace brave {
namespace {

using SavedTabURLTest = PlatformTest;

TEST_F(SavedTabURLTest, KeepsAnHttpURLAndItsTitle) {
  const auto [url, title] =
      NormalizeURLForSavedTabGroup(GURL("https://example.com/a"), u"Example");
  EXPECT_EQ(url, GURL("https://example.com/a"));
  EXPECT_EQ(title, u"Example");
}

TEST_F(SavedTabURLTest, UnwrapsReaderMode) {
  const auto [url, title] = NormalizeURLForSavedTabGroup(
      GURL("internal://local/reader-mode?url=https%3A%2F%2Fexample.com%2Fa"),
      u"Article");
  EXPECT_EQ(url, GURL("https://example.com/a"));
  EXPECT_EQ(title, u"Article");
}

TEST_F(SavedTabURLTest, UnwrapsAnInternalPage) {
  const auto [url, title] = NormalizeURLForSavedTabGroup(
      GURL("internal://local/blocked?url=https%3A%2F%2Fexample.com%2Fb"),
      u"Blocked");
  EXPECT_EQ(url, GURL("https://example.com/b"));
  EXPECT_EQ(title, u"Blocked");
}

TEST_F(SavedTabURLTest, MapsTheBraveNewTabPage) {
  const auto [url, title] =
      NormalizeURLForSavedTabGroup(GURL("about://newtab"), u"New Tab");
  EXPECT_EQ(url, GURL("chrome://newtab/"));
  EXPECT_EQ(title, u"New Tab");
}

TEST_F(SavedTabURLTest, MapsTheLegacyNewTabPage) {
  const auto [url, title] = NormalizeURLForSavedTabGroup(
      GURL("internal://local/about/home"), u"Home");
  EXPECT_EQ(url, GURL("chrome://newtab/"));
  EXPECT_EQ(title, u"Home");
}

TEST_F(SavedTabURLTest, ReplacesAnUnsaveableURL) {
  const auto [default_url, default_title] = tab_groups::GetDefaultUrlAndTitle();
  const auto [url, title] =
      NormalizeURLForSavedTabGroup(GURL("brave://settings"), u"Settings");
  EXPECT_EQ(url, default_url);
  EXPECT_EQ(title, default_title);
}

TEST_F(SavedTabURLTest, ReplacesAMissingURL) {
  const auto [default_url, default_title] = tab_groups::GetDefaultUrlAndTitle();
  const auto [url, title] = NormalizeURLForSavedTabGroup(GURL(), u"Missing");
  EXPECT_EQ(url, default_url);
  EXPECT_EQ(title, default_title);
}

}  // namespace
}  // namespace brave
