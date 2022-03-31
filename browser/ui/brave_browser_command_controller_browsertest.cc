/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "brave/components/brave_vpn/features.h"
#endif

class BraveBrowserCommandControllerTest : public InProcessBrowserTest {
 public:
  BraveBrowserCommandControllerTest() {
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

  void CheckBraveVPNCommands(Browser* browser) {
    // Only IDC_BRAVE_VPN_MENU command is changed based on purchased state.
    auto* command_controller = browser->command_controller();
    SetPurchasedUserForBraveVPN(browser, false);
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL));
    EXPECT_TRUE(command_controller->IsCommandEnabled(
        IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK));
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ABOUT_BRAVE_VPN));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN));

    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_BRAVE_VPN_MENU));
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_BRAVE_VPN));

    SetPurchasedUserForBraveVPN(browser, true);
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL));
    EXPECT_TRUE(command_controller->IsCommandEnabled(
        IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK));
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ABOUT_BRAVE_VPN));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN));

    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_BRAVE_VPN_MENU));
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_TOGGLE_BRAVE_VPN));
  }

  base::test::ScopedFeatureList scoped_feature_list_;
#endif
};

// Regular window
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTest) {
  // Test normal browser's brave commands status.
  auto* command_controller = browser()->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#else
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  CheckBraveVPNCommands(browser());
#endif

  if (syncer::IsSyncAllowedByFlag())
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  else
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));
}

// Create private browser and test its brave commands status.
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTestPrivateWindow) {
  auto* private_browser = CreateIncognitoBrowser();
  auto* command_controller = private_browser->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  CheckBraveVPNCommands(private_browser);
#endif

  if (syncer::IsSyncAllowedByFlag())
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  else
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));
}

// Create guest browser and test its brave commands status.
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTestGuestWindow) {
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(base::DoNothing());

  Browser* guest_browser = browser_creation_observer.Wait();
  DCHECK(guest_browser);
  EXPECT_TRUE(guest_browser->profile()->IsGuestSession());
  auto* command_controller = guest_browser->command_controller();
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  CheckBraveVPNCommands(guest_browser);
#endif

  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));
}

// Launch tor window and check its command status.
#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTestPrivateTorWindow) {
  ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  brave::NewOffTheRecordWindowTor(browser());
  Browser* tor_browser = tor_browser_creation_observer.Wait();
  DCHECK(tor_browser);
  EXPECT_TRUE(tor_browser->profile()->IsTor());
  auto* command_controller = tor_browser->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));

  if (syncer::IsSyncAllowedByFlag())
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  else
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  CheckBraveVPNCommands(tor_browser);
#endif

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));

  // Check tor commands when tor is disabled.
  TorProfileServiceFactory::SetTorDisabled(true);
  command_controller = browser()->command_controller();
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
}
#endif
