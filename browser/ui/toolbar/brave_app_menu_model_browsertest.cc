/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include <memory>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/common/pref_names.h"
#include "brave/common/tor/pref_names.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_browser_main.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"

using BraveAppMenuBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTest, BasicTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  BraveAppMenuModel normal_model(browser_view->toolbar(), browser());
  normal_model.Init();

  // Check normal window has both menu items.
  // -1 means |model| doesn't have passed command id.
  EXPECT_NE(-1, normal_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_NE(-1, normal_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_SYNC));
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_NE(-1, normal_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_WALLET));
#else
  EXPECT_EQ(-1, normal_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_WALLET));
#endif

  auto* command_controller = browser()->command_controller();
#if BUILDFLAG(ENABLE_TOR)
  EXPECT_NE(
      -1, normal_model.GetIndexOfCommandId(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
  // Check tor browser commands are disabled.
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
#else
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
#endif

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
#else
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#else
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#endif

  // Create proviate browser.
  auto* private_browser = CreateIncognitoBrowser();
  auto* private_browser_view =
      BrowserView::GetBrowserViewForBrowser(private_browser);
  BraveAppMenuModel private_model(private_browser_view->toolbar(),
                                  private_browser);
  private_model.Init();

  // Check private window has both menu items.
  // -1 means |model| doesn't have passed command id.
  EXPECT_NE(-1, private_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_NE(-1, private_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_SYNC));

  command_controller = private_browser->command_controller();
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
#else
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
#endif
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
#else
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
#endif

  content::WindowedNotificationObserver browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  profiles::SwitchToGuestProfile(ProfileManager::CreateCallback());

  browser_creation_observer.Wait();

  auto* browser_list = BrowserList::GetInstance();
  Browser* guest_browser = nullptr;
  for (Browser* browser : *browser_list) {
    if (browser->profile()->IsGuestSession()) {
      guest_browser = browser;
      break;
    }
  }
  DCHECK(guest_browser);

  auto* guest_browser_view =
      BrowserView::GetBrowserViewForBrowser(guest_browser);
  BraveAppMenuModel guest_model(guest_browser_view->toolbar(), guest_browser);
  guest_model.Init();

  // Check guest window doesn't have them.
  // -1 means |model| doesn't have passed command id.
  EXPECT_EQ(-1, guest_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_EQ(-1, guest_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_SYNC));
  EXPECT_EQ(-1, guest_model.GetIndexOfCommandId(IDC_SHOW_BRAVE_WALLET));

  command_controller = guest_browser->command_controller();
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
}

#if BUILDFLAG(ENABLE_TOR)
class ChromeBrowserMainExtraPartsTor : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsTor() = default;

  // ChromeBrowserMainExtraParts:
  void PostProfileInit() override {
    g_browser_process->local_state()->SetBoolean(tor::prefs::kTorDisabled,
                                                 true);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsTor);
};

class BraveAppMenuBrowserTestWithTorDisabledPolicy
    : public InProcessBrowserTest {
 public:
  void CreatedBrowserMainParts(content::BrowserMainParts* parts) override {
    static_cast<ChromeBrowserMainParts*>(parts)->AddParts(
        new ChromeBrowserMainExtraPartsTor);
  }
};

// If tor is disabled, corresponding menu and commands should be also disabled.
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTestWithTorDisabledPolicy,
                       TorDisabledTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  BraveAppMenuModel normal_model(browser_view->toolbar(), browser());
  normal_model.Init();

  // -1 means |model| doesn't have passed command id.
  EXPECT_EQ(
      -1, normal_model.GetIndexOfCommandId(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
  auto* command_controller = browser()->command_controller();
  // Check tor browser commands are disabled.
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
}
#endif
