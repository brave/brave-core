/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using ui_test_utils::BrowserChangeObserver;

enum class ProfileType { kRegular, kGuest, kPrivate, kTor };

const char* GetProfileTypeString(ProfileType type) {
  switch (type) {
    case ProfileType::kRegular:
      return "Regular";
    case ProfileType::kGuest:
      return "Guest";
    case ProfileType::kPrivate:
      return "Private";
    case ProfileType::kTor:
      return "Tor";
  }
  NOTREACHED();
}
}  // namespace

class AIChatProfileTest : public InProcessBrowserTest,
                          public ::testing::WithParamInterface<ProfileType> {
 public:
  AIChatProfileTest() = default;
  ~AIChatProfileTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    browser_ = CreateBrowser();
    ASSERT_TRUE(browser_);
  }

  Browser* CreateBrowser() {
    switch (GetParam()) {
      case ProfileType::kRegular:
        return browser();
      case ProfileType::kGuest:
        profiles::SwitchToGuestProfile();
        return ui_test_utils::WaitForBrowserToOpen();
        ;
      case ProfileType::kPrivate:
        return CreateIncognitoBrowser();
      case ProfileType::kTor: {
        BrowserChangeObserver observer(
            nullptr, BrowserChangeObserver::ChangeType::kAdded);
        brave::NewOffTheRecordWindowTor(browser());
        return observer.Wait();
      }
    }
    NOTREACHED();
  }

  bool IsAIChatEnabled() { return GetParam() == ProfileType::kRegular; }

  content::WebContents* web_contents() const {
    return browser_->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  raw_ptr<Browser, DanglingUntriaged> browser_ = nullptr;
};

IN_PROC_BROWSER_TEST_P(AIChatProfileTest, SidebarCheck) {
  auto* sidebar_model =
      static_cast<BraveBrowser*>(browser_)->sidebar_controller()->model();

  bool is_in_sidebar = base::ranges::any_of(
      sidebar_model->GetAllSidebarItems(), [](const auto& item) {
        return item.built_in_item_type ==
               sidebar::SidebarItem::BuiltInItemType::kChatUI;
      });
  if (IsAIChatEnabled()) {
    EXPECT_TRUE(is_in_sidebar);
  } else {
    EXPECT_FALSE(is_in_sidebar);
  }
}

IN_PROC_BROWSER_TEST_P(AIChatProfileTest, Autocomplete) {
  auto* autocomplete_controller = browser_->window()
                                      ->GetLocationBar()
                                      ->GetOmniboxView()
                                      ->controller()
                                      ->autocomplete_controller();
  const auto& providers = autocomplete_controller->providers();
  bool is_in_providers =
      base::ranges::any_of(providers, [](const auto& provider) {
        return provider->type() == AutocompleteProvider::TYPE_BRAVE_LEO;
      });
  if (IsAIChatEnabled()) {
    EXPECT_TRUE(is_in_providers);
  } else {
    EXPECT_FALSE(is_in_providers);
  }
}

IN_PROC_BROWSER_TEST_P(AIChatProfileTest, ContextMenu) {
  content::ContextMenuParams params;
  params.is_editable = false;
  params.page_url = GURL("http://test.page/");
  params.selection_text = u"brave";
  TestRenderViewContextMenu menu(*web_contents()->GetPrimaryMainFrame(),
                                 params);
  menu.Init();
  std::optional<size_t> ai_chat_index =
      menu.menu_model().GetIndexOfCommandId(IDC_AI_CHAT_CONTEXT_LEO_TOOLS);
  if (IsAIChatEnabled()) {
    EXPECT_TRUE(ai_chat_index.has_value());
  } else {
    EXPECT_FALSE(ai_chat_index.has_value());
  }
}

IN_PROC_BROWSER_TEST_P(AIChatProfileTest, SidePanelRegistry) {
  auto* registry = browser_->GetActiveTabInterface()
                       ->GetTabFeatures()
                       ->side_panel_registry();
  auto* entry = registry->GetEntryForKey(
      SidePanelEntry::Key(SidePanelEntry::Id::kChatUI));
  if (IsAIChatEnabled()) {
    EXPECT_TRUE(entry);
  } else {
    EXPECT_FALSE(entry);
  }
}

IN_PROC_BROWSER_TEST_P(AIChatProfileTest, SpeedreaderToolbar) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser_, GURL(base::StrCat({content::kChromeUIScheme, "://",
                                   kSpeedreaderPanelHost}))));
  auto result =
      content::EvalJs(web_contents(), "loadTimeData.data_.aiChatFeatureEnabled")
          .ExtractBool();
  if (IsAIChatEnabled()) {
    EXPECT_EQ(result, true);
  } else {
    EXPECT_EQ(result, false);
  }
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AIChatProfileTest,
    testing::ValuesIn({ProfileType::kRegular, ProfileType::kGuest,
                       ProfileType::kPrivate, ProfileType::kTor}),
    [](const testing::TestParamInfo<AIChatProfileTest::ParamType>& info) {
      return GetProfileTypeString(info.param);
    });
