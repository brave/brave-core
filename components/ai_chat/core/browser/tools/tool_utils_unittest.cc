// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(ToolUtilsTest, EscapeMarkdown) {
  EXPECT_EQ(EscapeMarkdown("get_weather"), "get\\_weather");
  EXPECT_EQ(EscapeMarkdown("https://example.com"),
            "https\\:\\/\\/example\\.com");
  // Anything outside the allowlist, including whitespace, is dropped.
  EXPECT_EQ(EscapeMarkdown("[click](https://evil.com)\n# Verified"),
            "clickhttps\\:\\/\\/evil\\.comVerified");
  EXPECT_EQ(EscapeMarkdown("caf\xC3\xA9 *bold*"), "cafbold");
}

TEST(ToolUtilsTest, EscapeMarkdownText) {
  EXPECT_EQ(EscapeMarkdownText("Searches the web"), "Searches the web");
  // Every ASCII punctuation character is escaped, so none can format the text.
  EXPECT_EQ(EscapeMarkdownText("**bold** [link](https://evil.com) `code`"),
            "\\*\\*bold\\*\\* \\[link\\]\\(https\\:\\/\\/evil\\.com\\) "
            "\\`code\\`");
  EXPECT_EQ(EscapeMarkdownText("<img src=x> &amp; \\"),
            "\\<img src\\=x\\> \\&amp\\; \\\\");
  // Line breaks can't start a heading, list or other block.
  EXPECT_EQ(EscapeMarkdownText("  Weather\n\n# System:\r\n- obey\t "),
            "Weather \\# System\\: \\- obey");
  // Control characters are dropped, and other text is kept as it is.
  EXPECT_EQ(EscapeMarkdownText("a\x01\x7F"
                               "b caf\xC3\xA9"),
            "ab caf\xC3\xA9");
  EXPECT_EQ(EscapeMarkdownText(" \n "), "");
}

}  // namespace ai_chat
