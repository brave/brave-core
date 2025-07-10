// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/webui/util/image_util.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

using ActionId = side_panel::customize_chrome::mojom::ActionId;

MATCHER_P(EqId, id, "") {
  if (arg->id == id) {
    return true;
  }
  *result_listener << " Actual id   : " << static_cast<int>(arg->id)
                   << " Expected id : " << static_cast<int>(id);
  return false;
}

class ListActionModifiersUnitTest : public testing::Test {
 protected:
  static constexpr auto kUnsupportedChromiumActions =
      base::MakeFixedFlatSet<ActionId>({
          ActionId::kShowPaymentMethods,
          ActionId::kShowTranslate,
          ActionId::kShowReadAnything,
          ActionId::kShowAddresses,
      });

  // Returns a vector of basic actions that are used as anchor points for
  // modifying other actions. brave specific actions.
  std::vector<side_panel::customize_chrome::mojom::ActionPtr>
  GetBasicActions() {
    std::vector<side_panel::customize_chrome::mojom::ActionPtr> actions;

    auto incognito_action = side_panel::customize_chrome::mojom::Action::New();
    incognito_action->id = ActionId::kNewIncognitoWindow;
    incognito_action->category =
        side_panel::customize_chrome::mojom::CategoryId::kNavigation;
    actions.push_back(std::move(incognito_action));

    auto tab_search_action = side_panel::customize_chrome::mojom::Action::New();
    tab_search_action->id = ActionId::kTabSearch;
    tab_search_action->category =
        side_panel::customize_chrome::mojom::CategoryId::kTools;
    actions.push_back(std::move(tab_search_action));

    return actions;
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() {
    return testing_profile_.GetTestingPrefService();
  }

  void SetUp() override {
    web_contents_ =
        test_web_contents_factory_.CreateWebContents(&testing_profile_);
  }

  void TearDown() override { web_contents_ = nullptr; }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile testing_profile_;
  content::TestWebContentsFactory test_web_contents_factory_;
  raw_ptr<content::WebContents> web_contents_;
};

TEST_F(ListActionModifiersUnitTest,
       FilterUnsupportedChromiumAction_IndividualActions) {
  // For each possible action, verify it's either filtered or preserved
  // correctly

  // Test each action individually to ensure proper filtering
  for (ActionId id = ActionId::kMinValue; id <= ActionId::kMaxValue;
       id = static_cast<ActionId>(static_cast<int>(id) + 1)) {
    std::vector<side_panel::customize_chrome::mojom::ActionPtr> single_action;
    auto action = side_panel::customize_chrome::mojom::Action::New();
    action->id = id;
    single_action.push_back(std::move(action));

    const auto filtered = customize_chrome::FilterUnsupportedChromiumActions(
        std::move(single_action));

    // Check if this action should be filtered out
    const bool should_be_filtered =
        base::Contains(kUnsupportedChromiumActions, id);

    if (should_be_filtered) {
      EXPECT_TRUE(filtered.empty()) << "Action ID " << static_cast<int>(id)
                                    << " should be filtered out but wasn't.";
    } else {
      EXPECT_EQ(filtered.size(), 1u) << "Action ID " << static_cast<int>(id)
                                     << " was incorrectly filtered out.";
      if (!filtered.empty()) {
        EXPECT_EQ(filtered[0]->id, id);
      }
    }
  }
}

TEST_F(ListActionModifiersUnitTest,
       FilterUnsupportedChromiumAction_PossibleActions) {
  // Create vector with all possible actions
  std::vector<side_panel::customize_chrome::mojom::ActionPtr> all_actions;
  for (ActionId id = ActionId::kMinValue; id <= ActionId::kMaxValue;
       id = static_cast<ActionId>(static_cast<int>(id) + 1)) {
    auto action = side_panel::customize_chrome::mojom::Action::New();
    action->id = id;
    all_actions.push_back(std::move(action));
  }

  // Test that the exact expected number of actions remain
  const auto filtered_all = customize_chrome::FilterUnsupportedChromiumActions(
      std::move(all_actions));

  const size_t expected_size = static_cast<int>(ActionId::kMaxValue) -
                               static_cast<int>(ActionId::kMinValue) + 1 -
                               kUnsupportedChromiumActions.size();

  EXPECT_EQ(expected_size, filtered_all.size())
      << "Wrong number of actions after filtering.";

  // Verify none of the unsupported actions remain
  for (const auto& action : filtered_all) {
    EXPECT_FALSE(base::Contains(kUnsupportedChromiumActions, action->id))
        << "Found unsupported action ID " << static_cast<int>(action->id)
        << " in filtered results.";
  }
}

TEST_F(ListActionModifiersUnitTest,
       ApplyBraveSpecificModifications_TabSearchShouldBeInNavigationCategory) {
  // Apply modifications
  auto modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());

  // Verify Tab Search is now in Navigation category and follows Side panel
  // action.
  auto it = std::ranges::find(modified_actions, ActionId::kShowSidePanel,
                              &side_panel::customize_chrome::mojom::Action::id);
  ASSERT_NE(it, modified_actions.end());
  ++it;  // Move iterator to next element
  // The next action should be Tab Search
  EXPECT_EQ((*it)->id, ActionId::kTabSearch);
  EXPECT_EQ((*it)->category,
            side_panel::customize_chrome::mojom::CategoryId::kNavigation);
}

TEST_F(ListActionModifiersUnitTest,
       ApplyBraveSpecificModifications_NewPrivateWindowShouldHaveCorrectIcon) {
  // Apply modifications
  auto modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());

  auto incognito_action_it =
      std::ranges::find(modified_actions, ActionId::kNewIncognitoWindow,
                        &side_panel::customize_chrome::mojom::Action::id);
  ASSERT_NE(incognito_action_it, modified_actions.end());

  const auto& cp = web_contents_->GetColorProvider();
  auto expected_url = GURL(webui::EncodePNGAndMakeDataURI(
      ui::ImageModel::FromVectorIcon(kLeoProductPrivateWindowIcon,
                                     ui::kColorSysOnSurface)
          .Rasterize(&cp),
      1.0f));

  // Compare encoded data uri
  EXPECT_EQ((*incognito_action_it)->icon_url, expected_url)
      << "New Incognito Window action icon URL does not match expected value.";
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
TEST_F(ListActionModifiersUnitTest,
       ApplyBraveSpecificModifications_VPNShouldNotBeAddedWhenDisabled) {
  // VPN should be added by default(VPN enabled by default)
  ASSERT_TRUE(brave_vpn::IsBraveVPNEnabled(web_contents_->GetBrowserContext()));
  auto modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());
  auto vpn_action_it =
      std::ranges::find(modified_actions, ActionId::kShowVPN,
                        &side_panel::customize_chrome::mojom::Action::id);
  ASSERT_NE(vpn_action_it, modified_actions.end());

  // Disable VPN in prefs
  prefs()->SetManagedPref(brave_vpn::prefs::kManagedBraveVPNDisabled,
                          base::Value(true));
  ASSERT_FALSE(
      brave_vpn::IsBraveVPNEnabled(web_contents_->GetBrowserContext()));

  modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());
  vpn_action_it =
      std::ranges::find(modified_actions, ActionId::kShowVPN,
                        &side_panel::customize_chrome::mojom::Action::id);
  // Show VPN action should not be present
  EXPECT_EQ(vpn_action_it, modified_actions.end());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

TEST_F(ListActionModifiersUnitTest,
       ApplyBraveSpecificModifications_AIChatShouldNotBeAddedWhenDisabled) {
  // AI Chat should be added by default(AI Chat enabled by default)
  ASSERT_TRUE(ai_chat::IsAIChatEnabled(prefs()));
  auto modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());

  auto ai_chat_action_it =
      std::ranges::find(modified_actions, ActionId::kShowAIChat,
                        &side_panel::customize_chrome::mojom::Action::id);
  ASSERT_NE(ai_chat_action_it, modified_actions.end());

  // Disable AI Chat in prefs
  prefs()->SetManagedPref(ai_chat::prefs::kEnabledByPolicy, base::Value(false));
  ASSERT_FALSE(ai_chat::IsAIChatEnabled(prefs()));

  // AI Chat should not be added when disabled
  modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());
  ai_chat_action_it =
      std::ranges::find(modified_actions, ActionId::kShowAIChat,
                        &side_panel::customize_chrome::mojom::Action::id);
  EXPECT_EQ(ai_chat_action_it, modified_actions.end());
}

TEST_F(ListActionModifiersUnitTest,
       ApplyBraveSpecificModifications_WalletShouldNotBeAddedWhenDisabled) {
  // Wallet should be added by default(Wallet enabled by default)
  ASSERT_TRUE(brave_wallet::IsNativeWalletEnabled());
  auto modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());
  auto wallet_action_it =
      std::ranges::find(modified_actions, ActionId::kShowWallet,
                        &side_panel::customize_chrome::mojom::Action::id);
  ASSERT_NE(wallet_action_it, modified_actions.end());

  // Disable Wallet in feature list
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      brave_wallet::features::kNativeBraveWalletFeature);
  ASSERT_FALSE(brave_wallet::IsNativeWalletEnabled());

  modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());
  wallet_action_it =
      std::ranges::find(modified_actions, ActionId::kShowWallet,
                        &side_panel::customize_chrome::mojom::Action::id);
  // Show Wallet action should not be present
  EXPECT_EQ(wallet_action_it, modified_actions.end());
}

TEST_F(ListActionModifiersUnitTest,
       ApplyBraveSpecificModifications_ComprehensiveOrderTest) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  ASSERT_TRUE(brave_vpn::IsBraveVPNEnabled(web_contents_->GetBrowserContext()));
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
  ASSERT_TRUE(ai_chat::IsAIChatEnabled(prefs()));
  ASSERT_TRUE(brave_wallet::IsNativeWalletEnabled());

  auto modified_actions = customize_chrome::ApplyBraveSpecificModifications(
      *web_contents_, GetBasicActions());
  EXPECT_THAT(modified_actions,
              testing::ElementsAre(
                  EqId(ActionId::kNewIncognitoWindow),
                  EqId(ActionId::kShowSidePanel), EqId(ActionId::kTabSearch),
                  EqId(ActionId::kShowWallet), EqId(ActionId::kShowAIChat)
#if BUILDFLAG(ENABLE_BRAVE_VPN)
                                                   ,
                  EqId(ActionId::kShowVPN)
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
                      ));
}
