/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_ask_helper.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/common/omnibox_features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_search {

class BraveSearchAskHelperTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(omnibox::kRouteQuestionsToAskBrave);
  }

  AutocompleteMatch CreateMatch(const GURL& url,
                                const std::u16string& contents) {
    AutocompleteMatch match;
    match.destination_url = url;
    match.contents = contents;
    return match;
  }

  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_EmptyString) {
  EXPECT_FALSE(IsQuestionInput(""));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_WhitespaceOnly) {
  EXPECT_FALSE(IsQuestionInput("   "));
  EXPECT_FALSE(IsQuestionInput("\t\n"));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_EndsWithQuestionMark) {
  EXPECT_TRUE(IsQuestionInput("test query?"));
  EXPECT_TRUE(IsQuestionInput("brave browser?"));
  EXPECT_TRUE(IsQuestionInput("?"));
  EXPECT_TRUE(IsQuestionInput("  spaces before and after  ?  "));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_QuestionWords) {
  // Basic question words
  EXPECT_TRUE(IsQuestionInput("what is brave"));
  EXPECT_TRUE(IsQuestionInput("where is the file"));
  EXPECT_TRUE(IsQuestionInput("when does it start"));
  EXPECT_TRUE(IsQuestionInput("why should I care"));
  EXPECT_TRUE(IsQuestionInput("who is that"));
  EXPECT_TRUE(IsQuestionInput("how do I install"));
  EXPECT_TRUE(IsQuestionInput("which one is better"));

  // Auxiliary verbs
  EXPECT_TRUE(IsQuestionInput("can you help me"));
  EXPECT_TRUE(IsQuestionInput("could this work"));
  EXPECT_TRUE(IsQuestionInput("would it be possible"));
  EXPECT_TRUE(IsQuestionInput("should I do this"));
  EXPECT_TRUE(IsQuestionInput("will this work"));
  EXPECT_TRUE(IsQuestionInput("is this correct"));
  EXPECT_TRUE(IsQuestionInput("are they coming"));
  EXPECT_TRUE(IsQuestionInput("was it good"));
  EXPECT_TRUE(IsQuestionInput("were you there"));
  EXPECT_TRUE(IsQuestionInput("do you know"));
  EXPECT_TRUE(IsQuestionInput("does it work"));
  EXPECT_TRUE(IsQuestionInput("did you see"));

  // With have
  EXPECT_TRUE(IsQuestionInput("have you tried"));
  EXPECT_TRUE(IsQuestionInput("has it been done"));
  EXPECT_TRUE(IsQuestionInput("had they arrived"));

  // Modal verbs
  EXPECT_TRUE(IsQuestionInput("may I come in"));
  EXPECT_TRUE(IsQuestionInput("might this be true"));
  EXPECT_TRUE(IsQuestionInput("shall we dance"));
  EXPECT_TRUE(IsQuestionInput("must I go"));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_NegativeContractions) {
  EXPECT_TRUE(IsQuestionInput("isn't it working"));
  EXPECT_TRUE(IsQuestionInput("aren't they here"));
  EXPECT_TRUE(IsQuestionInput("wasn't it good"));
  EXPECT_TRUE(IsQuestionInput("weren't you there"));
  EXPECT_TRUE(IsQuestionInput("doesn't it work"));
  EXPECT_TRUE(IsQuestionInput("didn't you see"));
  EXPECT_TRUE(IsQuestionInput("haven't you tried"));
  EXPECT_TRUE(IsQuestionInput("hasn't it happened"));
  EXPECT_TRUE(IsQuestionInput("hadn't they left"));
  EXPECT_TRUE(IsQuestionInput("won't you come"));
  EXPECT_TRUE(IsQuestionInput("wouldn't it be nice"));
  EXPECT_TRUE(IsQuestionInput("can't you see"));
  EXPECT_TRUE(IsQuestionInput("couldn't we try"));
  EXPECT_TRUE(IsQuestionInput("shouldn't I go"));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_CaseInsensitive) {
  EXPECT_TRUE(IsQuestionInput("What is brave"));
  EXPECT_TRUE(IsQuestionInput("WHAT IS BRAVE"));
  EXPECT_TRUE(IsQuestionInput("WhAt Is BrAvE"));
  EXPECT_TRUE(IsQuestionInput("HOW DO I INSTALL"));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_WithLeadingWhitespace) {
  EXPECT_TRUE(IsQuestionInput("  what is brave"));
  EXPECT_TRUE(IsQuestionInput("\twhere is the file"));
  EXPECT_TRUE(IsQuestionInput("   how do I install   "));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_NotQuestions) {
  EXPECT_FALSE(IsQuestionInput("brave browser"));
  EXPECT_FALSE(IsQuestionInput("install brave"));
  EXPECT_FALSE(IsQuestionInput("the what and why"));
  EXPECT_FALSE(IsQuestionInput("what"));
  EXPECT_FALSE(IsQuestionInput("something about how it works"));
  EXPECT_FALSE(IsQuestionInput("this is a statement"));
  EXPECT_FALSE(IsQuestionInput("question mark in middle? not at end"));
}

TEST_F(BraveSearchAskHelperTest, IsQuestionInput_QuestionWordNotAtStart) {
  EXPECT_FALSE(IsQuestionInput("I wonder what is brave"));
  EXPECT_FALSE(IsQuestionInput("tell me where it is"));
  EXPECT_FALSE(IsQuestionInput("the how and why"));
}

TEST_F(BraveSearchAskHelperTest, TransformsValidBraveSearchQuestion) {
  GURL url("https://search.brave.com/search?q=what+is+brave");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/ask?q=what+is+brave");
}

TEST_F(BraveSearchAskHelperTest, TransformsQuestionWithQuestionMark) {
  GURL url("https://search.brave.com/search?q=is+this+working");
  auto match = CreateMatch(url, u"is this working?");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/ask?q=is+this+working");
}

TEST_F(BraveSearchAskHelperTest, DoesNotTransformNonQuestion) {
  GURL url("https://search.brave.com/search?q=brave+browser");
  auto match = CreateMatch(url, u"brave browser");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/search?q=brave+browser");
}

TEST_F(BraveSearchAskHelperTest, DoesNotTransformNonBraveSearchURL) {
  GURL url("https://www.google.com/search?q=what+is+brave");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://www.google.com/search?q=what+is+brave");
}

TEST_F(BraveSearchAskHelperTest, DoesNotTransformDifferentPath) {
  GURL url("https://search.brave.com/images?q=what+is+brave");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/images?q=what+is+brave");
}

TEST_F(BraveSearchAskHelperTest, DoesNotTransformInvalidURL) {
  GURL url("not-a-valid-url");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(), "");
}

TEST_F(BraveSearchAskHelperTest, DoesNotTransformNonHTTPURL) {
  GURL url("ftp://search.brave.com/search?q=what+is+brave");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(match.destination_url.spec(),
            "ftp://search.brave.com/search?q=what+is+brave");
}

TEST_F(BraveSearchAskHelperTest, PreservesHTTPSScheme) {
  GURL url("https://search.brave.com/search?q=what+is+brave");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_TRUE(match.destination_url.SchemeIs("https"));
}

TEST_F(BraveSearchAskHelperTest, PreservesQueryParameters) {
  GURL url(
      "https://search.brave.com/search?q=what+is+brave&source=omnibox&lang=en");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  EXPECT_EQ(
      match.destination_url.spec(),
      "https://search.brave.com/ask?q=what+is+brave&source=omnibox&lang=en");
}

class BraveSearchAskHelperFeatureDisabledTest
    : public BraveSearchAskHelperTest {
 protected:
  void SetUp() override {
    feature_list_.InitAndDisableFeature(omnibox::kRouteQuestionsToAskBrave);
  }

  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveSearchAskHelperFeatureDisabledTest,
       DoesNotTransformWhenFeatureDisabled) {
  GURL url("https://search.brave.com/search?q=what+is+brave");
  auto match = CreateMatch(url, u"what is brave");

  MaybeTransformDestinationUrlForQuestionInput(match);

  // Should remain unchanged when feature is disabled
  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/search?q=what+is+brave");
}

}  // namespace brave_search
