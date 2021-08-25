/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
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
#include "components/sync/driver/sync_driver_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

using BraveBrowserCommandControllerTest = InProcessBrowserTest;

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

  if (switches::IsSyncAllowedByFlag())
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  else
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#else
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#endif

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));

  // Create private browser and test its brave commands status.
  auto* private_browser = CreateIncognitoBrowser();
  command_controller = private_browser->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

  if (switches::IsSyncAllowedByFlag())
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  else
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#endif

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));

  // Create guest browser and test its brave commands status.
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(ProfileManager::CreateCallback());

  Browser* guest_browser = browser_creation_observer.Wait();
  DCHECK(guest_browser);
  EXPECT_TRUE(guest_browser->profile()->IsGuestSession());
  command_controller = guest_browser->command_controller();
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#endif

  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));

#if BUILDFLAG(ENABLE_TOR)
  // Launch tor window and check its command status.
  ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  brave::NewOffTheRecordWindowTor(browser());
  Browser* tor_browser = tor_browser_creation_observer.Wait();
  DCHECK(tor_browser);
  EXPECT_TRUE(tor_browser->profile()->IsTor());
  command_controller = tor_browser->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK));

  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));

  if (switches::IsSyncAllowedByFlag())
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  else
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
#endif

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
#endif
}
