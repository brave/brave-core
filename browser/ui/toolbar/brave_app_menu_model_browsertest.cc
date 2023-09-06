/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include <algorithm>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
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
#include "chrome/browser/ui/toolbar/recent_tabs_sub_menu_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/features.h"
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#endif

class BraveAppMenuModelBrowserTest : public InProcessBrowserTest {
 public:
  BraveAppMenuModelBrowserTest() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
#endif
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void SetPurchasedUserForBraveVPN(Browser* browser, bool purchased) {
    auto* service =
        brave_vpn::BraveVpnServiceFactory::GetForProfile(browser->profile());
    ASSERT_TRUE(!!service);
    auto target_state = purchased
                            ? brave_vpn::mojom::PurchasedState::PURCHASED
                            : brave_vpn::mojom::PurchasedState::NOT_PURCHASED;
    service->SetPurchasedState(skus::GetDefaultEnvironment(), target_state);
    // Call explicitely to update vpn commands status because mojo works in
    // async way.
    static_cast<chrome::BraveBrowserCommandController*>(
        browser->command_controller())
        ->OnPurchasedStateChanged(target_state, absl::nullopt);
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
  for (int id : disabled_commands) {
    EXPECT_FALSE(model->GetIndexOfCommandId(id).has_value());
  }
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
  std::vector<size_t> commands_index;
  for (int id : commands_in_order) {
    absl::optional<size_t> index = model->GetIndexOfCommandId(id);
    EXPECT_TRUE(index.has_value());
    commands_index.push_back(index.value());
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

void CheckMoreToolsCommandsAreInOrderInMenuModel(
    Browser* browser,
    const std::vector<int>& more_tools_commands_in_order) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveAppMenuModel model(browser_view->toolbar(), browser);
  model.Init();
  ui::SimpleMenuModel* more_tools_model =
      static_cast<ui::SimpleMenuModel*>(model.GetSubmenuModelAt(
          model.GetIndexOfCommandId(IDC_MORE_TOOLS_MENU).value()));
  CheckCommandsAreInOrderInMenuModel(more_tools_model,
                                     more_tools_commands_in_order);
}

void CheckMoreToolsCommandsAreDisabledInMenuModel(
    Browser* browser,
    const std::vector<int>& more_tools_disabled_commands) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveAppMenuModel model(browser_view->toolbar(), browser);
  model.Init();
  ui::SimpleMenuModel* more_tools_model =
      static_cast<ui::SimpleMenuModel*>(model.GetSubmenuModelAt(
          model.GetIndexOfCommandId(IDC_MORE_TOOLS_MENU).value()));
  CheckCommandsAreDisabledInMenuModel(more_tools_model,
                                      more_tools_disabled_commands);
}

void CheckHelpCommandsAreInOrderInMenuModel(
    Browser* browser,
    const std::vector<int>& help_commands_in_order) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveAppMenuModel model(browser_view->toolbar(), browser);
  model.Init();
  ui::SimpleMenuModel* help_model =
      static_cast<ui::SimpleMenuModel*>(model.GetSubmenuModelAt(
          model.GetIndexOfCommandId(IDC_HELP_MENU).value()));
  CheckCommandsAreInOrderInMenuModel(help_model, help_commands_in_order);
}

void CheckHistoryCommandsAreInOrderInMenuModel(
    Browser* browser,
    const std::vector<int>& history_commands_in_order) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveAppMenuModel model(browser_view->toolbar(), browser);
  model.Init();
  ui::SimpleMenuModel* history_model =
      static_cast<ui::SimpleMenuModel*>(model.GetSubmenuModelAt(
          model.GetIndexOfCommandId(IDC_RECENT_TABS_MENU).value()));
  CheckCommandsAreInOrderInMenuModel(history_model, history_commands_in_order);
}

// Test brave menu order test.
// Brave menu is inserted based on corresponding commands enable status.
// So, this doesn't test for each profiles(normal, private, tor and guest).
// Instead, BraveBrowserCommandControllerTest will do that.
IN_PROC_BROWSER_TEST_F(BraveAppMenuModelBrowserTest, MenuOrderTest) {
  std::vector<int> commands_in_order_for_normal_profile = {
    IDC_NEW_TAB,
    IDC_NEW_WINDOW,
    IDC_NEW_INCOGNITO_WINDOW,
#if BUILDFLAG(ENABLE_TOR)
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
#endif
    IDC_SHOW_BRAVE_WALLET,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
    IDC_RECENT_TABS_MENU,
    IDC_BOOKMARKS_MENU,
    IDC_SHOW_DOWNLOADS,
    IDC_MANAGE_EXTENSIONS,
    IDC_ZOOM_MENU,
    IDC_PRINT,
    IDC_FIND,
    IDC_MORE_TOOLS_MENU,
    IDC_EDIT_MENU,
    IDC_HELP_MENU,
    IDC_OPTIONS,
  };

  std::vector<int> commands_disabled_for_normal_profile = {
      IDC_NEW_TOR_CONNECTION_FOR_SITE,
  };
  CheckCommandsAreInOrderInMenuModel(browser(),
                                     commands_in_order_for_normal_profile);
  CheckCommandsAreDisabledInMenuModel(browser(),
                                      commands_disabled_for_normal_profile);

  // Same help menu is used for all profiles.
  std::vector<int> help_commands_in_order = {
      IDC_ABOUT,
      IDC_HELP_PAGE_VIA_MENU,
      IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER,
  };
  CheckHelpCommandsAreInOrderInMenuModel(browser(), help_commands_in_order);
  CheckHistoryCommandsAreInOrderInMenuModel(
      browser(), {IDC_SHOW_HISTORY, IDC_CLEAR_BROWSING_DATA});

  std::vector<int> more_tools_in_order = {
      IDC_ADD_NEW_PROFILE, IDC_OPEN_GUEST_PROFILE, IDC_SIDEBAR_SHOW_OPTION_MENU,
      IDC_SHOW_BRAVE_SYNC, IDC_DEV_TOOLS,          IDC_TASK_MANAGER,
  };

  if (!syncer::IsSyncAllowedByFlag()) {
    more_tools_in_order.erase(
        std::remove(more_tools_in_order.begin(), more_tools_in_order.end(),
                    IDC_SHOW_BRAVE_SYNC),
        commands_in_order_for_normal_profile.end());
    more_tools_in_order.push_back(IDC_SHOW_BRAVE_SYNC);
  }

  CheckMoreToolsCommandsAreInOrderInMenuModel(browser(), more_tools_in_order);

  auto* private_browser = CreateIncognitoBrowser();
  std::vector<int> commands_in_order_for_private_profile = {
    IDC_NEW_TAB,
    IDC_NEW_WINDOW,
    IDC_NEW_INCOGNITO_WINDOW,
#if BUILDFLAG(ENABLE_TOR)
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
#endif
    IDC_SHOW_BRAVE_WALLET,
    IDC_BOOKMARKS_MENU,
    IDC_SHOW_DOWNLOADS,
    IDC_MANAGE_EXTENSIONS,
    IDC_ZOOM_MENU,
    IDC_PRINT,
    IDC_FIND,
    IDC_MORE_TOOLS_MENU,
    IDC_EDIT_MENU,
    IDC_HELP_MENU,
    IDC_OPTIONS,
  };

  std::vector<int> commands_disabled_for_private_profile = {
    IDC_NEW_TOR_CONNECTION_FOR_SITE,
    IDC_RECENT_TABS_MENU,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
  };

  CheckCommandsAreInOrderInMenuModel(private_browser,
                                     commands_in_order_for_private_profile);
  CheckCommandsAreDisabledInMenuModel(private_browser,
                                      commands_disabled_for_private_profile);
  CheckHelpCommandsAreInOrderInMenuModel(private_browser,
                                         help_commands_in_order);
  CheckMoreToolsCommandsAreInOrderInMenuModel(private_browser,
                                              more_tools_in_order);

  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(base::DoNothing());

  Browser* guest_browser = browser_creation_observer.Wait();
  DCHECK(guest_browser);
  EXPECT_TRUE(guest_browser->profile()->IsGuestSession());
  std::vector<int> commands_in_order_for_guest_profile = {
      IDC_NEW_TAB,   IDC_NEW_WINDOW, IDC_SHOW_DOWNLOADS,  IDC_ZOOM_MENU,
      IDC_PRINT,     IDC_FIND,       IDC_MORE_TOOLS_MENU, IDC_EDIT_MENU,
      IDC_HELP_MENU, IDC_OPTIONS,
  };

  CheckCommandsAreInOrderInMenuModel(guest_browser,
                                     commands_in_order_for_guest_profile);
  std::vector<int> commands_disabled_for_guest_profile = {
    IDC_NEW_INCOGNITO_WINDOW,
#if BUILDFLAG(ENABLE_TOR)
    IDC_NEW_OFFTHERECORD_WINDOW_TOR,
#endif
    IDC_SHOW_BRAVE_WALLET,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
    IDC_RECENT_TABS_MENU,
    IDC_BOOKMARKS_MENU,
    IDC_MANAGE_EXTENSIONS,
  };

  CheckCommandsAreDisabledInMenuModel(guest_browser,
                                      commands_disabled_for_guest_profile);
  CheckHelpCommandsAreInOrderInMenuModel(guest_browser, help_commands_in_order);

  std::vector<int> more_tools_in_order_for_guest_profile = {
      IDC_SIDEBAR_SHOW_OPTION_MENU,
      IDC_DEV_TOOLS,
      IDC_TASK_MANAGER,
  };
  CheckMoreToolsCommandsAreInOrderInMenuModel(
      guest_browser, more_tools_in_order_for_guest_profile);

  std::vector<int> more_tools_disabled_for_guest_profile = {
      IDC_ADD_NEW_PROFILE,
      IDC_OPEN_GUEST_PROFILE,
      IDC_SHOW_BRAVE_SYNC,
  };
  CheckMoreToolsCommandsAreDisabledInMenuModel(
      guest_browser, more_tools_disabled_for_guest_profile);

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
      IDC_SHOW_BRAVE_WALLET,
      IDC_BOOKMARKS_MENU,
      IDC_SHOW_DOWNLOADS,
      IDC_MANAGE_EXTENSIONS,
      IDC_ZOOM_MENU,
      IDC_PRINT,
      IDC_FIND,
      IDC_MORE_TOOLS_MENU,
      IDC_EDIT_MENU,
      IDC_HELP_MENU,
      IDC_OPTIONS,
  };
  std::vector<int> commands_disabled_for_tor_profile = {
    IDC_RECENT_TABS_MENU,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    IDC_SHOW_BRAVE_VPN_PANEL,
#endif
  };
  CheckCommandsAreInOrderInMenuModel(tor_browser,
                                     commands_in_order_for_tor_profile);
  CheckCommandsAreDisabledInMenuModel(tor_browser,
                                      commands_disabled_for_tor_profile);
  CheckHelpCommandsAreInOrderInMenuModel(tor_browser, help_commands_in_order);
  CheckMoreToolsCommandsAreInOrderInMenuModel(tor_browser, more_tools_in_order);

#endif
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
// Check vpn menu based on purchased status.
IN_PROC_BROWSER_TEST_F(BraveAppMenuModelBrowserTest, BraveVPNMenuTest) {
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
IN_PROC_BROWSER_TEST_F(BraveAppMenuModelBrowserTest, BraveIpfsMenuTest) {
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

void CheckMenuIcons(ui::MenuModel* menu,
                    int submenu_depth,
                    std::u16string path = u"") {
  constexpr int kIconlessCommands[] = {
      // Header, with no icon
      IDC_RECENT_TABS_NO_DEVICE_TABS,
      // Header, with no icon
      RecentTabsSubMenuModel::kDisabledRecentlyClosedHeaderCommandId};
  for (size_t i = 0; i < menu->GetItemCount(); ++i) {
    auto command_id = menu->GetCommandIdAt(i);
    // Skip separators & commands which deliberately have no icons
    if (command_id == -1 || base::Contains(kIconlessCommands, command_id)) {
      continue;
    }

    auto label = menu->GetLabelAt(i);
    auto icon = menu->GetIconAt(i);
    EXPECT_FALSE(icon.IsEmpty()) << "\"" << path << label << "\""
                                 << " for Command Id: " << command_id
                                 << " (at index " << i << ") has no icon";

    if (auto* submenu = menu->GetSubmenuModelAt(i)) {
      if (submenu_depth > 0) {
        CheckMenuIcons(submenu, submenu_depth - 1, path + label + +u" > ");
      }
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveAppMenuModelBrowserTest, MenuItemsHaveIcons) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  BraveAppMenuModel model(browser_view->toolbar(), browser());
  model.Init();

  CheckMenuIcons(&model, 1);
}
