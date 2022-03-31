/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include <algorithm>
#include <vector>

#include "base/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "brave/components/brave_vpn/features.h"
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#endif

class BraveAppMenuBrowserTest : public InProcessBrowserTest {
 public:
  BraveAppMenuBrowserTest() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
#endif
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void SetPurchasedUserForBraveVPN(Browser* browser, bool purchased) {
    auto* service = BraveVpnServiceFactory::GetForProfile(browser->profile());
    auto target_state =
        purchased ? PurchasedState::PURCHASED : PurchasedState::NOT_PURCHASED;
    service->SetPurchasedState(target_state);
    // Call explicitely to update vpn commands status because mojo works in
    // async way.
    static_cast<chrome::BraveBrowserCommandController*>(
        browser->command_controller())
        ->OnPurchasedStateChanged(target_state);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  void SetIPNSKeys(Browser* browser, int count) {
    auto* ipfs_service_ =
        ipfs::IpfsServiceFactory::GetInstance()->GetForContext(
            browser->profile());
    ASSERT_TRUE(ipfs_service_);
    ipfs_service_->SetAllowIpfsLaunchForTest(true);
    std::unordered_map<std::string, std::string> keys;
    for (auto i = 0; i < count; i++) {
      auto value = std::to_string(i);
      keys[value] = value;
    }
    ipfs_service_->GetIpnsKeysManager()->SetKeysForTest(keys);
  }
#endif
};

void CheckCommandsAreDisabledInMenuModel(
    ui::SimpleMenuModel* model,
    const std::vector<int>& disabled_commands) {
  for (int id : disabled_commands)
    EXPECT_EQ(-1, model->GetIndexOfCommandId(id));
}

void CheckCommandsAreDisabledInMenuModel(
    Browser* browser,
    const std::vector<int>& disabled_commands) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveAppMenuModel model(browser_view->toolbar(), browser);
  model.Init();
  CheckCommandsAreDisabledInMenuModel(&model, disabled_commands);
}

void CheckIpfsCommandsAreDisabledForMode(Browser* browser,
                                         ipfs::IPFSResolveMethodTypes mode) {
  std::vector<int> commands_disabled = {IDC_APP_MENU_IPFS};
  browser->profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
                                             static_cast<int>(mode));

  CheckCommandsAreDisabledInMenuModel(browser, commands_disabled);
}

void CheckCommandsAreInOrderInMenuModel(
    ui::SimpleMenuModel* model,
    const std::vector<int>& commands_in_order) {
  std::vector<int> commands_index;
  for (int id : commands_in_order) {
    int index = model->GetIndexOfCommandId(id);
    EXPECT_NE(-1, index);
    commands_index.push_back(index);
  }
  EXPECT_TRUE(
      std::is_sorted(std::begin(commands_index), std::end(commands_index)));
}

void CheckCommandsAreInOrderInMenuModel(
    Browser* browser,
    const std::vector<int>& commands_in_order) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveAppMenuModel model(browser_view->toolbar(), browser);
  model.Init();
  CheckCommandsAreInOrderInMenuModel(&model, commands_in_order);
}

// Test brave menu order test.
// Brave menu is inserted based on corresponding commands enable status.
// So, this doesn't test for each profiles(normal, private, tor and guest).
// Instead, BraveBrowserCommandControllerTest will do that.
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTest, MenuOrderTest) {
  std::vector<int> commands_in_order_for_normal_profile = {
    IDC_NEW_TAB,
    IDC_NEW_WINDOW,
    IDC_NEW_INCOGNITO_WINDOW,
#if BUILDFLAG(ENABLE_TOR)
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
#endif
    IDC_SHOW_BRAVE_REWARDS,
    IDC_RECENT_TABS_MENU,
    IDC_BOOKMARKS_MENU,
    IDC_SHOW_DOWNLOADS,
    IDC_SHOW_BRAVE_WALLET,
    IDC_MANAGE_EXTENSIONS,
    IDC_SHOW_BRAVE_SYNC,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
    IDC_SHOW_BRAVE_ADBLOCK,
    IDC_ADD_NEW_PROFILE,
    IDC_OPEN_GUEST_PROFILE,
    IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER
  };
  std::vector<int> commands_disabled_for_normal_profile = {
      IDC_NEW_TOR_CONNECTION_FOR_SITE,
  };
  if (!syncer::IsSyncAllowedByFlag()) {
    commands_in_order_for_normal_profile.erase(
        std::remove(commands_in_order_for_normal_profile.begin(),
                    commands_in_order_for_normal_profile.end(),
                    IDC_SHOW_BRAVE_SYNC),
        commands_in_order_for_normal_profile.end());
    commands_disabled_for_normal_profile.push_back(IDC_SHOW_BRAVE_SYNC);
  }
  CheckCommandsAreInOrderInMenuModel(browser(),
                                     commands_in_order_for_normal_profile);
  CheckCommandsAreDisabledInMenuModel(browser(),
                                      commands_disabled_for_normal_profile);

  auto* private_browser = CreateIncognitoBrowser();
  std::vector<int> commands_in_order_for_private_profile = {
    IDC_NEW_TAB,
    IDC_NEW_WINDOW,
    IDC_NEW_INCOGNITO_WINDOW,
#if BUILDFLAG(ENABLE_TOR)
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
#endif
    IDC_SHOW_BRAVE_REWARDS,
    IDC_BOOKMARKS_MENU,
    IDC_SHOW_DOWNLOADS,
    IDC_SHOW_BRAVE_WALLET,
    IDC_MANAGE_EXTENSIONS,
    IDC_SHOW_BRAVE_SYNC,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
    IDC_SHOW_BRAVE_ADBLOCK,
    IDC_ADD_NEW_PROFILE,
    IDC_OPEN_GUEST_PROFILE,
    IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER
  };
  std::vector<int> commands_disabled_for_private_profile = {
      IDC_NEW_TOR_CONNECTION_FOR_SITE,
      IDC_RECENT_TABS_MENU,
  };
  if (!syncer::IsSyncAllowedByFlag()) {
    commands_in_order_for_private_profile.erase(
        std::remove(commands_in_order_for_private_profile.begin(),
                    commands_in_order_for_private_profile.end(),
                    IDC_SHOW_BRAVE_SYNC),
        commands_in_order_for_private_profile.end());
    commands_disabled_for_private_profile.push_back(IDC_SHOW_BRAVE_SYNC);
  }
  CheckCommandsAreInOrderInMenuModel(private_browser,
                                     commands_in_order_for_private_profile);
  CheckCommandsAreDisabledInMenuModel(private_browser,
                                      commands_disabled_for_private_profile);

  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(base::DoNothing());

  Browser* guest_browser = browser_creation_observer.Wait();
  DCHECK(guest_browser);
  EXPECT_TRUE(guest_browser->profile()->IsGuestSession());
  std::vector<int> commands_in_order_for_guest_profile = {
    IDC_NEW_TAB,
    IDC_NEW_WINDOW,
    IDC_SHOW_DOWNLOADS,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
    IDC_SHOW_BRAVE_ADBLOCK,
    IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER
  };
  CheckCommandsAreInOrderInMenuModel(guest_browser,
                                     commands_in_order_for_guest_profile);
  std::vector<int> commands_disabled_for_guest_profile = {
    IDC_NEW_INCOGNITO_WINDOW,
#if BUILDFLAG(ENABLE_TOR)
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
#endif
    IDC_SHOW_BRAVE_REWARDS,
    IDC_RECENT_TABS_MENU,
    IDC_BOOKMARKS_MENU,
    IDC_SHOW_BRAVE_WALLET,
    IDC_MANAGE_EXTENSIONS,
    IDC_ADD_NEW_PROFILE,
    IDC_OPEN_GUEST_PROFILE,
  };
  CheckCommandsAreDisabledInMenuModel(guest_browser,
                                      commands_disabled_for_guest_profile);

#if BUILDFLAG(ENABLE_TOR)
  ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  brave::NewOffTheRecordWindowTor(browser());
  Browser* tor_browser = tor_browser_creation_observer.Wait();
  DCHECK(tor_browser);
  EXPECT_TRUE(tor_browser->profile()->IsTor());
  std::vector<int> commands_in_order_for_tor_profile = {
    IDC_NEW_TAB,
    IDC_NEW_TOR_CONNECTION_FOR_SITE,
    IDC_NEW_WINDOW,
    IDC_NEW_INCOGNITO_WINDOW,
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
    IDC_SHOW_BRAVE_REWARDS,
    IDC_BOOKMARKS_MENU,
    IDC_SHOW_DOWNLOADS,
    IDC_SHOW_BRAVE_WALLET,
    IDC_SHOW_BRAVE_SYNC,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
    IDC_SHOW_BRAVE_ADBLOCK,
    IDC_ADD_NEW_PROFILE,
    IDC_OPEN_GUEST_PROFILE,
    IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER
  };
  std::vector<int> commands_disabled_for_tor_profile = {
      IDC_RECENT_TABS_MENU,
  };
  if (!syncer::IsSyncAllowedByFlag()) {
    commands_in_order_for_tor_profile.erase(
        std::remove(commands_in_order_for_tor_profile.begin(),
                    commands_in_order_for_tor_profile.end(),
                    IDC_SHOW_BRAVE_SYNC),
        commands_in_order_for_tor_profile.end());
    commands_disabled_for_tor_profile.push_back(IDC_SHOW_BRAVE_SYNC);
  }
  CheckCommandsAreInOrderInMenuModel(tor_browser,
                                     commands_in_order_for_tor_profile);
  CheckCommandsAreDisabledInMenuModel(tor_browser,
                                      commands_disabled_for_tor_profile);
#endif
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
// Check vpn menu based on purchased status.
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTest, BraveVPNMenuTest) {
  std::vector<int> commands_enabled_for_non_purchased = {
      IDC_SHOW_BRAVE_VPN_PANEL,
  };
  std::vector<int> commands_disabled_for_non_purchased = {
      IDC_BRAVE_VPN_MENU,
  };

  SetPurchasedUserForBraveVPN(browser(), false);
  CheckCommandsAreInOrderInMenuModel(browser(),
                                     commands_enabled_for_non_purchased);
  CheckCommandsAreDisabledInMenuModel(browser(),
                                      commands_disabled_for_non_purchased);

  std::vector<int> commands_enabled_for_purchased = {
      IDC_BRAVE_VPN_MENU,
  };
  std::vector<int> commands_disabled_for_purchased = {
      IDC_SHOW_BRAVE_VPN_PANEL,
  };

  SetPurchasedUserForBraveVPN(browser(), true);
  CheckCommandsAreInOrderInMenuModel(browser(), commands_enabled_for_purchased);
  CheckCommandsAreDisabledInMenuModel(browser(),
                                      commands_disabled_for_purchased);
}
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTest, BraveIpfsMenuTest) {
  CheckIpfsCommandsAreDisabledForMode(
      browser(), ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
  CheckIpfsCommandsAreDisabledForMode(browser(),
                                      ipfs::IPFSResolveMethodTypes::IPFS_ASK);
  CheckIpfsCommandsAreDisabledForMode(
      browser(), ipfs::IPFSResolveMethodTypes::IPFS_DISABLED);
  browser()->profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  std::vector<int> commands_enabled_with_local_node = {IDC_APP_MENU_IPFS};
  {
    SetIPNSKeys(browser(), 0);
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    BraveAppMenuModel model(browser_view->toolbar(), browser());
    model.Init();

    CheckCommandsAreInOrderInMenuModel(&model,
                                       commands_enabled_with_local_node);

    std::vector<int> commands_enabled_without_keys = {
        IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE,
        IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER};
    CheckCommandsAreInOrderInMenuModel(&model.ipfs_submenu_model_,
                                       commands_enabled_without_keys);
    std::vector<int> commands_disabled_without_keys = {
        IDC_APP_MENU_IPFS_UPDATE_IPNS};
    CheckCommandsAreDisabledInMenuModel(&model.ipfs_submenu_model_,
                                        commands_disabled_without_keys);
  }
  {
    int count = 3;
    SetIPNSKeys(browser(), count);
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    BraveAppMenuModel model(browser_view->toolbar(), browser());
    model.Init();

    CheckCommandsAreInOrderInMenuModel(&model,
                                       commands_enabled_with_local_node);

    std::vector<int> commands_enabled_with_keys = {
        IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE,
        IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER, IDC_APP_MENU_IPFS_UPDATE_IPNS};
    CheckCommandsAreInOrderInMenuModel(&model.ipfs_submenu_model_,
                                       commands_enabled_with_keys);
    std::vector<int> submenu_commands_enabled_with_keys = {
        IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FILE,
        IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER};
    CheckCommandsAreInOrderInMenuModel(&model.ipns_submenu_model_,
                                       submenu_commands_enabled_with_keys);

    EXPECT_EQ(model.ipns_keys_submenu_models_.size(), 2u);

    int commands_start = IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START;
    {
      auto& submenu = model.ipns_keys_submenu_models_.at(
          IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FILE);
      std::vector<int> keys_commands_enabled;
      for (auto i = 0; i < count; i++) {
        keys_commands_enabled.push_back(commands_start++);
      }

      CheckCommandsAreInOrderInMenuModel(submenu.get(), keys_commands_enabled);
    }
    {
      auto& submenu = model.ipns_keys_submenu_models_.at(
          IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER);
      std::vector<int> keys_commands_enabled;
      for (auto i = 0; i < count; i++) {
        keys_commands_enabled.push_back(commands_start++);
      }

      CheckCommandsAreInOrderInMenuModel(submenu.get(), keys_commands_enabled);
    }
  }
}
#endif
