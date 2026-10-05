// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/passage_splitter.h"

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

using Passages = std::vector<std::string>;

TEST(PassageSplitterTest, NoPassagesWithoutWords) {
  EXPECT_TRUE(SplitIntoPassages("", 10, 100).empty());
  EXPECT_TRUE(SplitIntoPassages(" \n\t\n ", 10, 100).empty());
}

TEST(PassageSplitterTest, TrimsShortText) {
  EXPECT_EQ(SplitIntoPassages("  hello world \n", 10, 100),
            Passages{"hello world"});
}

TEST(PassageSplitterTest, PacksParagraphsWhileTheyFit) {
  EXPECT_EQ(SplitIntoPassages("one two\n\nthree four\n\nfive six", 4, 100),
            (Passages{"one two\n\nthree four", "five six"}));
}

TEST(PassageSplitterTest, StartsNewPassageForParagraphThatDoesNotFit) {
  EXPECT_EQ(SplitIntoPassages("a b c\n\nd e f", 4, 100),
            (Passages{"a b c", "d e f"}));
}

TEST(PassageSplitterTest, SplitsLongParagraphBetweenWords) {
  EXPECT_EQ(SplitIntoPassages("a b c d e", 2, 100),
            (Passages{"a b", "c d", "e"}));
  // The rest of a split paragraph is packed with the next one.
  EXPECT_EQ(SplitIntoPassages("a b c\n\nd", 2, 100),
            (Passages{"a b", "c\n\nd"}));
}

TEST(PassageSplitterTest, KeepsLineBreaks) {
  EXPECT_EQ(SplitIntoPassages("first line\nsecond line", 10, 100),
            Passages{"first line\nsecond line"});
  // A line of spaces still separates paragraphs.
  EXPECT_EQ(SplitIntoPassages("a\n  \nb c", 2, 100), (Passages{"a", "b c"}));
}

TEST(PassageSplitterTest, LimitsBytes) {
  EXPECT_EQ(SplitIntoPassages("aaaa bbbb", 10, 4), (Passages{"aaaa", "bbbb"}));
  EXPECT_EQ(SplitIntoPassages("aaaaaaaaa", 10, 4),
            (Passages{"aaaa", "aaaa", "a"}));
}

TEST(PassageSplitterTest, SplitsUnspacedTextBetweenCharacters) {
  // Four characters of three bytes each.
  constexpr char kText[] = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\xE3\x81\xAE";
  const Passages passages = SplitIntoPassages(kText, 10, 7);
  EXPECT_EQ(passages,
            (Passages{"\xE6\x97\xA5\xE6\x9C\xAC", "\xE8\xAA\x9E\xE3\x81\xAE"}));
  for (const std::string& passage : passages) {
    EXPECT_TRUE(base::IsStringUTF8(passage));
  }
}

}  // namespace ai_chat
