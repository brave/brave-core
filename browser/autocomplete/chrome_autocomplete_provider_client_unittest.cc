/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "testing/gtest/include/gtest/gtest.h"

class AutocompleteControllerLeoTest : public testing::Test {
 public:
  AutocompleteControllerLeoTest() = default;

  // Creates a "Leo" match in the result.
  static AutocompleteMatch MakeLeoMatch(const std::u16string& contents,
                                        int relevance) {
    AutocompleteMatch match(nullptr, relevance, false,
                            AutocompleteMatchType::SEARCH_SUGGEST_ENTITY);
    match.contents = contents;
    match.RecordAdditionalInfo("match-from-brave-leo-provider", true);
    return match;
  }

  // Creates a "normal" search or URL match.
  static AutocompleteMatch MakeRegularMatch(const std::u16string& contents,
                                            int relevance) {
    AutocompleteMatch match(nullptr, relevance, false,
                            AutocompleteMatchType::SEARCH_SUGGEST);
    match.contents = contents;
    return match;
  }
};

TEST_F(AutocompleteControllerLeoTest, MaybeShowLeoMatchOrdering) {
  AutocompleteResult result;

  result.AppendMatches({
      MakeLeoMatch(u"LeoMatch", /*relevance=*/1700),
      MakeRegularMatch(u"This Is The Way", /*relevance=*/1600),
      MakeRegularMatch(u"Make it so", /*relevance=*/1500),
  });

  ASSERT_EQ(result.size(), 3U);
  EXPECT_EQ(result.match_at(0)->contents, u"LeoMatch");

  {
    // When AIChatFirst is disabled, the Leo match to be forced to bottom.
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeatures(
        /*enabled_features=*/{},
        /*disabled_features=*/{ai_chat::features::kAIChatFirst});

    ai_chat::MaybeShowLeoMatch(&result);
    EXPECT_EQ(result.match_at(2)->contents, u"LeoMatch")
        << "When AIChatFirst is off, Leo match should be at the bottom.";
  }

  // Reset the ordering back to how it was originally.
  result.Reset();
  result.AppendMatches({
      MakeLeoMatch(u"LeoMatch", 1700),
      MakeRegularMatch(u"This Is The Way", 1600),
      MakeRegularMatch(u"Make it so", 1500),
  });

  {
    // When AIChatFirst is enabled, the Leo match should be first
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeatures(
        /*enabled_features=*/{ai_chat::features::kAIChatFirst},
        /*disabled_features=*/{});

    ai_chat::MaybeShowLeoMatch(&result);
    EXPECT_EQ(result.size(), 3U);
    EXPECT_EQ(result.match_at(0)->contents, u"LeoMatch")
        << "When AIChatFirst is on, Leo match is NOT forced down.";
  }
}

#endif  // BUILDFLAG(ENABLE_AI_CHAT)
