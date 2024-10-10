// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/commander_provider.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/observer_list.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#include "brave/components/commander/browser/commander_item_model.h"
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#include "brave/components/omnibox/browser/brave_fake_autocomplete_provider_client.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/range/range.h"

namespace {

class FakeCommanderDelegate : public commander::CommanderFrontendDelegate {
 public:
  FakeCommanderDelegate() = default;
  ~FakeCommanderDelegate() override {}

  void AddObserver(Observer* observer) override {
    Notify(items_);
    observers_.AddObserver(observer);
  }
  void RemoveObserver(Observer* observer) override {
    observers_.RemoveObserver(observer);
  }

  void Toggle() override { showing_ = !showing_; }

  void Hide() override { showing_ = false; }

  std::vector<commander::CommandItemModel> GetItems() override {
    return items_;
  }

  int GetResultSetId() override { return notifies_; }

  const std::u16string& GetPrompt() override { return prompt_; }

  void Notify(const std::vector<commander::CommandItemModel>& items,
              std::u16string prompt = std::u16string()) {
    items_ = items;
    notifies_++;
    prompt_ = std::move(prompt);

    for (auto& observer : observers_) {
      observer.OnCommanderUpdated();
    }
  }

  void SelectCommand(uint32_t command_index, uint32_t result_set_id) override {
    NOTIMPLEMENTED();
  }

  void UpdateText(const std::u16string& text) override { NOTIMPLEMENTED(); }

 private:
  base::ObserverList<Observer> observers_;
  std::u16string prompt_;
  std::vector<commander::CommandItemModel> items_;
  int notifies_ = 0;
  bool showing_ = false;
};

AutocompleteInput CreateInput(std::u16string text) {
  return AutocompleteInput(text,
                           metrics::OmniboxEventProto::PageClassification::
                               OmniboxEventProto_PageClassification_NTP,
                           TestSchemeClassifier());
}
}  // namespace

class CommanderProviderTest : public testing::Test {
 public:
  CommanderProviderTest() {
    features_.InitAndEnableFeature(features::kBraveCommander);
  }

  void SetUp() override {
    client_.set_commander_delegate(std::make_unique<FakeCommanderDelegate>());
    provider_ =
        base::MakeRefCounted<commander::CommanderProvider>(&client_, nullptr);
  }

  FakeCommanderDelegate* delegate() {
    return static_cast<FakeCommanderDelegate*>(client_.GetCommanderDelegate());
  }
  commander::CommanderProvider* provider() { return provider_.get(); }

 private:
  base::test::ScopedFeatureList features_;

  BraveFakeAutocompleteProviderClient client_;
  scoped_refptr<commander::CommanderProvider> provider_;
};

TEST_F(CommanderProviderTest, EmptyTextDoesNotTriggerProvider) {
  delegate()->Notify({commander::CommandItemModel(u"First", {}, u"Ctrl+F")});

  provider()->Start(CreateInput(u""), false);
  EXPECT_EQ(0u, provider()->matches().size());
}

TEST_F(CommanderProviderTest, NonPrefixedTextDoesNotTriggerProvider) {
  provider()->Start(CreateInput(u"Hello"), false);
  EXPECT_EQ(0u, provider()->matches().size());
}

TEST_F(CommanderProviderTest, PrefixTriggersProvider) {
  provider()->Start(CreateInput(u""), false);
  EXPECT_EQ(0u, provider()->matches().size());

  provider()->Start(CreateInput(commander::kCommandPrefix.data()), false);
  EXPECT_EQ(0u, provider()->matches().size());
}

TEST_F(CommanderProviderTest, PrefixedCommandTriggersProvider) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u"Hello"})), false);
  EXPECT_EQ(0u, provider()->matches().size());
}

TEST_F(CommanderProviderTest, PrefixWhiteSpaceIsStripped) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u"  Hello"})),
      false);
  EXPECT_EQ(0u, provider()->matches().size());
}

TEST_F(CommanderProviderTest, ItemsAreConvertedToMatches) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u" Hello World"})),
      false);

  delegate()->Notify({commander::CommandItemModel(u"First", {}, u"Ctrl+F"),
                      commander::CommandItemModel(u"Second", {}, u"Ctrl+S")});

  EXPECT_EQ(2u, provider()->matches().size());

  EXPECT_EQ(u"First", provider()->matches()[0].description);
  EXPECT_EQ(u"Ctrl+F", provider()->matches()[0].contents);

  EXPECT_EQ(u"Second", provider()->matches()[1].description);
  EXPECT_EQ(u"Ctrl+S", provider()->matches()[1].contents);

  for (const auto& match : provider()->matches()) {
    // As we haven't specified a prompt, none of the matches should have
    // additional_text.
    EXPECT_EQ(u"", match.additional_text);
    // All matches swap the contents & description, so that the command displays
    // on the left and the shortcut (if any) on the right.
    EXPECT_TRUE(match.swap_contents_and_description);
    // All matches should be allowed to be the default, so when the user presses
    // enter, the top command is executed.
    EXPECT_TRUE(match.allowed_to_be_default_match);
    // fill_into_edit should be the same as whatever the last input was, so
    // scrolling through the commands doesn't affect what the user typed.
    EXPECT_EQ(u":> Hello World", match.fill_into_edit);
    // Check the matches have actions.
    EXPECT_TRUE(match.takeover_action);
  }
}

TEST_F(CommanderProviderTest, RemovingPrefixClearsMatches) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u" Hello World"})),
      false);

  delegate()->Notify({commander::CommandItemModel(u"First", {}, u"Ctrl+F"),
                      commander::CommandItemModel(u"Second", {}, u"Ctrl+S")});
  EXPECT_EQ(2u, provider()->matches().size());

  provider()->Start(CreateInput(u"no prefix!"), false);
  EXPECT_EQ(0u, provider()->matches().size());
}

TEST_F(CommanderProviderTest, NoMatchRangeAllDimStyle) {
  provider()->Start(CreateInput(u":> Hello World"), false);

  delegate()->Notify({commander::CommandItemModel(u"Foo", {}, u"")},
                     u"What thing?");

  EXPECT_EQ(1u, provider()->matches().size());
  const auto& c = provider()->matches()[0].description_class;
  ASSERT_EQ(1u, c.size());
  EXPECT_EQ(0u, c[0].offset);
  EXPECT_EQ(ACMatchClassification::DIM, c[0].style);
}

TEST_F(CommanderProviderTest, ZeroCharMatchIsIgnored) {
  provider()->Start(CreateInput(u":> Hello World"), false);

  delegate()->Notify({commander::CommandItemModel(u"Foo", {gfx::Range()}, u"")},
                     u"What thing?");

  EXPECT_EQ(1u, provider()->matches().size());
  const auto& c = provider()->matches()[0].description_class;
  ASSERT_EQ(1u, c.size());
  EXPECT_EQ(0u, c[0].offset);
  EXPECT_EQ(ACMatchClassification::DIM, c[0].style);
}

TEST_F(CommanderProviderTest, OneCharMatchIsHighlighted) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u" Hello World"})),
      false);

  delegate()->Notify(
      {commander::CommandItemModel(u"Foo", {gfx::Range(0, 1)}, u"")},
      u"What thing?");

  EXPECT_EQ(1u, provider()->matches().size());
  const auto& c = provider()->matches()[0].description_class;
  ASSERT_EQ(2u, c.size());

  // F is MATCH
  EXPECT_EQ(0u, c[0].offset);
  EXPECT_EQ(ACMatchClassification::MATCH, c[0].style);

  // oo should be DIM, as it didn't match
  EXPECT_EQ(1u, c[1].offset);
  EXPECT_EQ(ACMatchClassification::DIM, c[1].style);
}

// Note: The AutocompleteClassifier gets unhappy if the style switches back and
// forth on the same character (i.e one match finishes where another one
// starts).
TEST_F(CommanderProviderTest, AdjacentMatchesDontSwitchBackAndForth) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u" Hello World"})),
      false);

  delegate()->Notify({commander::CommandItemModel(
                         u"Foo", {gfx::Range(0, 1), gfx::Range(1, 2)}, u"")},
                     u"What thing?");

  const auto& c = provider()->matches()[0].description_class;
  EXPECT_EQ(1u, provider()->matches().size());
  ASSERT_EQ(3u, c.size());

  // F is MATCH, but shouldn't add a closing DIM
  EXPECT_EQ(0u, c[0].offset);
  EXPECT_EQ(ACMatchClassification::MATCH, c[0].style);

  // first o is also MATCH
  EXPECT_EQ(1u, c[1].offset);
  EXPECT_EQ(ACMatchClassification::MATCH, c[1].style);

  // second o should be DIM, as it didn't match
  EXPECT_EQ(2u, c[2].offset);
  EXPECT_EQ(ACMatchClassification::DIM, c[2].style);
}

TEST_F(CommanderProviderTest, FullLengthMatchIsApplied) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u" Hello World"})),
      false);

  delegate()->Notify(
      {commander::CommandItemModel(u"Foo", {gfx::Range(0, 3)}, u"")},
      u"What thing?");

  const auto& c = provider()->matches()[0].description_class;
  EXPECT_EQ(1u, provider()->matches().size());
  ASSERT_EQ(1u, c.size());

  // Foo is MATCH
  EXPECT_EQ(0u, c[0].offset);
  EXPECT_EQ(ACMatchClassification::MATCH, c[0].style);
}

TEST_F(CommanderProviderTest, MatchesCanHaveGaps) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u"FoBa"})), false);

  delegate()->Notify(
      {commander::CommandItemModel(u"Foo Bar",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u"")},
      u"What thing?");

  const auto& c = provider()->matches()[0].description_class;
  EXPECT_EQ(1u, provider()->matches().size());
  ASSERT_EQ(4u, c.size());

  // |Fo|o Bar is MATCH
  EXPECT_EQ(0u, c[0].offset);
  EXPECT_EQ(ACMatchClassification::MATCH, c[0].style);

  EXPECT_EQ(2u, c[1].offset);
  EXPECT_EQ(ACMatchClassification::DIM, c[1].style);

  // Foo |Ba|r is MATCH
  EXPECT_EQ(4u, c[2].offset);
  EXPECT_EQ(ACMatchClassification::MATCH, c[2].style);

  EXPECT_EQ(6u, c[3].offset);
  EXPECT_EQ(ACMatchClassification::DIM, c[3].style);
}

TEST_F(CommanderProviderTest, MatchesHaveCustomIcon) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u"FoBa"})), false);

  delegate()->Notify(
      {commander::CommandItemModel(u"Foo Bar",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u""),
       commander::CommandItemModel(u"Fizz Bazz",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u"")},
      u"What thing?");

  EXPECT_EQ(2u, provider()->matches().size());
  for (const auto& result : provider()->matches()) {
    EXPECT_EQ(&kLeoCaratRightIcon, &result.GetVectorIcon(false, nullptr));
  }
}

TEST_F(CommanderProviderTest, ExplicitMatchesDoNotHaveGroup) {
  provider()->Start(
      CreateInput(base::StrCat({commander::kCommandPrefix, u"FoBa"})), false);

  delegate()->Notify(
      {commander::CommandItemModel(u"Foo Bar",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u""),
       commander::CommandItemModel(u"Fizz Bazz",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u"")},
      u"What thing?");

  EXPECT_LT(0u, provider()->matches().size());
  for (const auto& match : provider()->matches()) {
    EXPECT_EQ(std::nullopt, match.suggestion_group_id);
  }
}

TEST_F(CommanderProviderTest, MatchesInAmbientModeHaveGroup) {
  provider()->Start(CreateInput(u"FoBa"), false);

  delegate()->Notify(
      {commander::CommandItemModel(u"Foo Bar",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u""),
       commander::CommandItemModel(u"Fizz Bazz",
                                   {gfx::Range(0, 2), gfx::Range(4, 6)}, u"")},
      u"What thing?");

  EXPECT_LT(0u, provider()->matches().size());
  for (const auto& match : provider()->matches()) {
    EXPECT_EQ(omnibox::GroupId::GROUP_OTHER_NAVS, match.suggestion_group_id);
  }
}
