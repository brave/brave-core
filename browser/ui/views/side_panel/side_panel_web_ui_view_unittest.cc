/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_web_ui_view_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(SidePanelWebUIViewUtilsTest,
     ShouldEnableContextMenu_ReturnsTrueForAIChat) {
  // Test that ShouldEnableContextMenu returns true for AI Chat URL
  GURL ai_chat_url(kAIChatUIURL);
  bool result = brave::ShouldEnableContextMenu(ai_chat_url);
  EXPECT_TRUE(result);
}

TEST(SidePanelWebUIViewUtilsTest,
     ShouldEnableContextMenu_ReturnsFalseForNonAIChat) {
  // Test that ShouldEnableContextMenu returns false for non-AI Chat URL
  GURL bookmarks_url("chrome://bookmarks-side-panel.top-chrome/");
  bool result = brave::ShouldEnableContextMenu(bookmarks_url);
  EXPECT_FALSE(result);
}

TEST(SidePanelWebUIViewUtilsTest,
     ShouldEnableContextMenu_ReturnsFalseForInvalidURL) {
  // Test that ShouldEnableContextMenu returns false for invalid URL
  GURL invalid_url;
  bool result = brave::ShouldEnableContextMenu(invalid_url);
  EXPECT_FALSE(result);
}

TEST(SidePanelWebUIViewUtilsTest,
     ShouldEnableContextMenu_ReturnsFalseForEmptyURL) {
  // Test that ShouldEnableContextMenu returns false for empty URL
  GURL empty_url("");
  bool result = brave::ShouldEnableContextMenu(empty_url);
  EXPECT_FALSE(result);
}
