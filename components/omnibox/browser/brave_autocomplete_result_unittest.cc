// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/omnibox/browser/actions/omnibox_action_concepts.h"
#include "components/omnibox/browser/actions/tab_switch_action.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/common/omnibox_features.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveAutocompleteResultTest : public testing::Test {
 public:
  BraveAutocompleteResultTest() = default;
  ~BraveAutocompleteResultTest() override = default;
};

AutocompleteMatch CreateMatch(const GURL& url, bool has_tab_match) {
  AutocompleteMatch result(nullptr, 0, false,
                           AutocompleteMatchType::HISTORY_TITLE);
  result.contents = base::UTF8ToUTF16(url.spec());
  result.destination_url = url;
  result.has_tab_match = has_tab_match;
  if (has_tab_match) {
    result.actions.push_back(base::MakeRefCounted<TabSwitchAction>(url));
  }
  return result;
}

TEST_F(BraveAutocompleteResultTest, OmniboxTabSwitchByDefaultIsDisabled) {
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(omnibox::kOmniboxTabSwitchByDefault));
}

TEST_F(BraveAutocompleteResultTest,
       ResultsWithTabMatchAreNotTouchedWhenAutoTabSwitchFeatureDisabled) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(omnibox::kOmniboxTabSwitchByDefault);

  AutocompleteResult result;
  result.AppendMatches({
      CreateMatch(GURL("https://example.com/1"), true),
      CreateMatch(GURL("https://example.com/2"), true),
  });
  result.ConvertOpenTabMatches(nullptr, nullptr);

  EXPECT_EQ(2u, result.size());
  for (size_t i = 0; i < result.size(); ++i) {
    auto* match = result.match_at(i);
    EXPECT_TRUE(match->has_tab_match.value_or(false));
    EXPECT_EQ(1u, match->actions.size());
    EXPECT_EQ(OmniboxActionId::TAB_SWITCH, match->actions[0]->ActionId());
    EXPECT_FALSE(match->takeover_action);
  }
}

TEST_F(BraveAutocompleteResultTest,
       TabMatchBecomesDefaultActionWhenAutoTabSwitchFeatureEnabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(omnibox::kOmniboxTabSwitchByDefault);

  AutocompleteResult result;
  result.AppendMatches({
      CreateMatch(GURL("https://example.com/1"), true),
      CreateMatch(GURL("https://example.com/2"), true),
  });
  result.ConvertOpenTabMatches(nullptr, nullptr);

  EXPECT_EQ(2u, result.size());
  for (size_t i = 0; i < result.size(); ++i) {
    auto* match = result.match_at(i);
    EXPECT_TRUE(match->has_tab_match.value_or(false));
    EXPECT_EQ(1u, match->actions.size());
    EXPECT_EQ(OmniboxActionId::UNKNOWN, match->actions[0]->ActionId());
    EXPECT_TRUE(match->takeover_action);
    EXPECT_EQ(OmniboxActionId::TAB_SWITCH, match->takeover_action->ActionId());
  }
}

TEST_F(BraveAutocompleteResultTest,
       NonTabMatchesAreNotTouchedWhenAutoTabSwitchFeatureEnabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(omnibox::kOmniboxTabSwitchByDefault);

  AutocompleteResult result;
  result.AppendMatches({
      CreateMatch(GURL("https://example.com/1"), true),
      CreateMatch(GURL("https://foo.com/1"), false),
      CreateMatch(GURL("https://example.com/1"), true),
      CreateMatch(GURL("https://foo.com/2"), false),
  });
  result.ConvertOpenTabMatches(nullptr, nullptr);

  EXPECT_EQ(4u, result.size());
  for (size_t i = 0; i < result.size(); ++i) {
    auto* match = result.match_at(i);

    // In this test, even numbered results are tab matches.
    bool should_be_match = i % 2 == 0;

    if (should_be_match) {
      EXPECT_TRUE(match->has_tab_match.value_or(false));
      EXPECT_EQ(1u, match->actions.size());
      EXPECT_EQ(OmniboxActionId::UNKNOWN, match->actions[0]->ActionId());
      EXPECT_TRUE(match->takeover_action);
      EXPECT_EQ(OmniboxActionId::TAB_SWITCH,
                match->takeover_action->ActionId());
    } else {
      EXPECT_FALSE(match->has_tab_match.value_or(false));
      EXPECT_EQ(0u, match->actions.size());
      EXPECT_FALSE(match->takeover_action);
    }
  }
}
