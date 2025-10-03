// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ai_chat/enabled_state_transition_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class AIChatEnabledStateTransitionServiceBrowserTest
    : public InProcessBrowserTest {
 public:
  AIChatEnabledStateTransitionServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(ai_chat::features::kAIChat);
  }

  void SetUpInProcessBrowserTestFixture() override {
    EnabledStateTransitionServiceFactory::GetInstance();
    provider_.SetDefaultReturns(true, true);
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  ~AIChatEnabledStateTransitionServiceBrowserTest() override = default;

  Profile* profile() { return browser()->profile(); }

  void SetPolicyEnabled(bool enabled) {
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveAIChatEnabled,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, base::Value(enabled), nullptr);
    provider_.UpdateChromePolicy(policies);
    base::RunLoop().RunUntilIdle();
  }

 protected:
  GURL ai_chat_url_ = GURL(kAIChatUIURL);
  testing::NiceMock<policy::MockConfigurationPolicyProvider> provider_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(AIChatEnabledStateTransitionServiceBrowserTest,
                       TabsClosedWhenDisabled) {
  // Open multiple tabs - two with AI Chat and one with brave.com
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), ai_chat_url_));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), ai_chat_url_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://brave.com"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  // Verify we have 3 tabs
  auto* tab_strip = browser()->tab_strip_model();
  ASSERT_EQ(tab_strip->count(), 3);

  // Open the ChatUI sidebar panel for the brave.com tab
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(panel_ui);
  panel_ui->Show(SidePanelEntryId::kChatUI);
  ASSERT_TRUE(panel_ui->GetCurrentEntryId().has_value());
  ASSERT_EQ(panel_ui->GetCurrentEntryId().value(), SidePanelEntryId::kChatUI);

  // Disable AI Chat via policy - this should close the two AI Chat tabs and
  // close the sidebar panel
  SetPolicyEnabled(false);

  // Verify only 1 tab remains (the brave.com tab)
  EXPECT_EQ(tab_strip->count(), 1);
  EXPECT_EQ(tab_strip->GetActiveWebContents()->GetLastCommittedURL(),
            GURL("https://brave.com"));

  // Verify the ChatUI sidebar panel is no longer shown
  EXPECT_FALSE(panel_ui->GetCurrentEntryId().has_value());
}

IN_PROC_BROWSER_TEST_F(AIChatEnabledStateTransitionServiceBrowserTest,
                       SidebarItemToggledWithPolicy) {
  auto* sidebar_service =
      sidebar::SidebarServiceFactory::GetForProfile(profile());
  ASSERT_TRUE(sidebar_service);

  // Initially, AI Chat should be in the visible items (policy enabled by
  // default)
  const auto& initial_items = sidebar_service->items();
  auto iter = std::ranges::find(initial_items,
                                sidebar::SidebarItem::BuiltInItemType::kChatUI,
                                &sidebar::SidebarItem::built_in_item_type);
  EXPECT_NE(iter, initial_items.end());

  // Disable AI Chat via policy - this should remove it from sidebar
  SetPolicyEnabled(false);

  const auto& items_after_disable = sidebar_service->items();
  iter = std::ranges::find(items_after_disable,
                           sidebar::SidebarItem::BuiltInItemType::kChatUI,
                           &sidebar::SidebarItem::built_in_item_type);
  EXPECT_EQ(iter, items_after_disable.end());

  // Re-enable AI Chat via policy - this should add it back to sidebar
  SetPolicyEnabled(true);

  const auto& items_after_enable = sidebar_service->items();
  iter = std::ranges::find(items_after_enable,
                           sidebar::SidebarItem::BuiltInItemType::kChatUI,
                           &sidebar::SidebarItem::built_in_item_type);
  EXPECT_NE(iter, items_after_enable.end());
}

}  // namespace ai_chat
