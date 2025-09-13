/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <string>

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

// Test for FormatPageContentWithTitle utility function
TEST(EngineConsumerTest, FormatPageContentWithTitle_FormatsContentWithTitle) {
  // Test with both title and content
  PageContent content_with_title(u"My Page Title", "This is the page content",
                                 false);
  std::string result = FormatPageContentWithTitle(content_with_title);
  EXPECT_EQ(result, "Title: My Page Title\nThis is the page content");
}

TEST(EngineConsumerTest,
     FormatPageContentWithTitle_FormatsContentWithEmptyTitle) {
  // Test with empty title
  PageContent content_empty_title(u"", "This is content without title", false);
  std::string result = FormatPageContentWithTitle(content_empty_title);
  EXPECT_EQ(result, "Title: \nThis is content without title");
}

TEST(EngineConsumerTest, FormatPageContentWithTitle_FormatsVideoContent) {
  // Test with video content
  PageContent video_content(u"Video Title", "This is video transcript content",
                            true);
  std::string result = FormatPageContentWithTitle(video_content);
  EXPECT_EQ(result, "Title: Video Title\nThis is video transcript content");
}

TEST(EngineConsumerTest,
     FormatPageContentWithTitle_FormatsContentWithSpecialCharacters) {
  // Test with special characters in title
  PageContent content_special(u"Title with émojis 🚀 & symbols",
                              "Content with special chars: <>&\"'", false);
  std::string result = FormatPageContentWithTitle(content_special);
  EXPECT_EQ(result,
            "Title: Title with émojis 🚀 & symbols\nContent with special "
            "chars: <>&\"'");
}

TEST(EngineConsumerTest,
     FormatPageContentWithTitle_FormatsContentWithMultilineContent) {
  // Test with multiline content
  PageContent multiline_content(u"Multiline Title", "Line 1\nLine 2\nLine 3",
                                false);
  std::string result = FormatPageContentWithTitle(multiline_content);
  EXPECT_EQ(result, "Title: Multiline Title\nLine 1\nLine 2\nLine 3");
}

}  // namespace ai_chat
