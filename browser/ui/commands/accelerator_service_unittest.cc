// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/accelerator_service.h"

#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "brave/components/commands/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/tor/pref_names.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

namespace commands {

class AcceleratorServiceUnitTest : public testing::Test {
 public:
  AcceleratorServiceUnitTest() {
    features_.InitAndEnableFeature(commands::features::kBraveCommands);
  }

  ~AcceleratorServiceUnitTest() override = default;

  TestingProfile& profile() { return profile_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  base::test::ScopedFeatureList features_;
};

TEST_F(AcceleratorServiceUnitTest, CanOverrideExistingShortcut) {
  commands::AcceleratorService service(
      profile().GetPrefs(),
      {{IDC_NEW_TAB, {commands::FromCodesString("Control+KeyT")}}}, {});

  auto accelerators = service.GetAcceleratorsForTesting();
  ASSERT_EQ(1u, accelerators[IDC_NEW_TAB].size());
  EXPECT_EQ("Control+KeyT",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][0]));

  service.AssignAcceleratorToCommand(IDC_NEW_WINDOW, "Control+KeyT");
  accelerators = service.GetAcceleratorsForTesting();
  EXPECT_EQ(0u, accelerators[IDC_NEW_TAB].size());
  ASSERT_EQ(1u, accelerators[IDC_NEW_WINDOW].size());
  EXPECT_EQ("Control+KeyT",
            commands::ToCodesString(accelerators[IDC_NEW_WINDOW][0]));
}

TEST_F(AcceleratorServiceUnitTest, AcceleratorsArePersisted) {
  commands::AcceleratorService service(profile().GetPrefs(), {}, {});

  auto accelerators = service.GetAcceleratorsForTesting();
  service.AssignAcceleratorToCommand(IDC_NEW_TAB, "Control+KeyT");

  commands::AcceleratorService service2(profile().GetPrefs(), {}, {});
  accelerators = service2.GetAcceleratorsForTesting();

  ASSERT_EQ(1u, accelerators[IDC_NEW_TAB].size());
  EXPECT_EQ("Control+KeyT",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][0]));
}

TEST_F(AcceleratorServiceUnitTest, AcceleratorsCanBeRemoved) {
  commands::AcceleratorService service(
      profile().GetPrefs(),
      {{IDC_NEW_TAB, {commands::FromCodesString("Control+KeyT")}}}, {});
  service.AssignAcceleratorToCommand(IDC_NEW_TAB, "Control+KeyK");
  auto accelerators = service.GetAcceleratorsForTesting();
  EXPECT_EQ(2u, accelerators[IDC_NEW_TAB].size());

  // Unassigning a non-existent command should be a no-op.
  service.UnassignAcceleratorFromCommand(IDC_NEW_TAB, "Control+KeyA");
  accelerators = service.GetAcceleratorsForTesting();
  EXPECT_EQ(2u, accelerators[IDC_NEW_TAB].size());

  service.UnassignAcceleratorFromCommand(IDC_NEW_TAB, "Control+KeyT");
  accelerators = service.GetAcceleratorsForTesting();
  EXPECT_EQ(1u, accelerators[IDC_NEW_TAB].size());

  service.UnassignAcceleratorFromCommand(IDC_NEW_TAB, "Control+KeyU");
  accelerators = service.GetAcceleratorsForTesting();
  EXPECT_EQ(1u, accelerators[IDC_NEW_TAB].size());
}

TEST_F(AcceleratorServiceUnitTest, AcceleratorsCanBeReset) {
  commands::AcceleratorService service(
      profile().GetPrefs(),
      {{IDC_NEW_TAB,
        {commands::FromCodesString("Control+KeyT"),
         commands::FromCodesString("Control+KeyK"),
         commands::FromCodesString("Control+KeyU")}}},
      {});

  auto accelerators = service.GetAcceleratorsForTesting();
  ASSERT_EQ(3u, accelerators[IDC_NEW_TAB].size());

  // Add one new command
  service.AssignAcceleratorToCommand(IDC_NEW_TAB, "Control+KeyJ");

  // Remove one command
  service.UnassignAcceleratorFromCommand(IDC_NEW_TAB, "Control+KeyK");

  // Reassign one accelerator
  service.AssignAcceleratorToCommand(IDC_NEW_WINDOW, "Control+KeyT");
  accelerators = service.GetAcceleratorsForTesting();

  ASSERT_EQ(2u, accelerators[IDC_NEW_TAB].size());
  ASSERT_EQ(1u, accelerators[IDC_NEW_WINDOW].size());

  service.ResetAcceleratorsForCommand(IDC_NEW_TAB);
  accelerators = service.GetAcceleratorsForTesting();

  ASSERT_EQ(3u, accelerators[IDC_NEW_TAB].size());
  EXPECT_EQ("Control+KeyT",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][0]));
  EXPECT_EQ("Control+KeyK",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][1]));
  EXPECT_EQ("Control+KeyU",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][2]));
  EXPECT_EQ(0u, accelerators[IDC_NEW_WINDOW].size());
}

TEST_F(AcceleratorServiceUnitTest, DefaultAcceleratorsCanBeUpdated) {
  {
    commands::AcceleratorService service(
        profile().GetPrefs(),
        {{IDC_NEW_TAB,
          {commands::FromCodesString("Control+KeyT"),
           commands::FromCodesString("Control+KeyQ")}},
         {IDC_NEW_WINDOW, {commands::FromCodesString("Control+KeyN")}}},
        {});
    service.AssignAcceleratorToCommand(IDC_NEW_TAB, "Control+KeyJ");
    service.AssignAcceleratorToCommand(IDC_NEW_WINDOW, "Control+KeyW");
  }

  // In the new commands service, the following changes have been made:
  // 1) Remove |Control+KeyQ| from IDC_NEW_TAB
  // 2) Add |Control+KeyY| to IDC_NEW_TAB
  // 3) Add |Control+KeyW| to IDC_NEW_TAB (replacing the shortcut from
  // IDC_NEW_WINDOW)
  // 4) Remove default accelerators fro IDC_NEW_WINDOW 4) Add
  // 5) New default accelerator to IDC_PIN_TARGET_TAB
  commands::AcceleratorService new_service(
      profile().GetPrefs(),
      {{{IDC_NEW_TAB,
         {commands::FromCodesString("Control+KeyT"),
          commands::FromCodesString("Control+KeyY"),
          commands::FromCodesString("Control+KeyW")}},
        {IDC_WINDOW_PIN_TAB, {commands::FromCodesString("Alt+KeyP")}}}},
      {});
  auto accelerators = new_service.GetAcceleratorsForTesting();
  ASSERT_EQ(4u, accelerators[IDC_NEW_TAB].size());
  EXPECT_EQ("Control+KeyT",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][0]));
  EXPECT_EQ("Control+KeyJ",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][1]));
  EXPECT_EQ("Control+KeyY",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][2]));
  EXPECT_EQ("Control+KeyW",
            commands::ToCodesString(accelerators[IDC_NEW_TAB][3]));

  EXPECT_EQ(0u, accelerators[IDC_NEW_WINDOW].size());

  ASSERT_EQ(1u, accelerators[IDC_WINDOW_PIN_TAB].size());
  EXPECT_EQ("Alt+KeyP",
            commands::ToCodesString(accelerators[IDC_WINDOW_PIN_TAB][0]));
}

TEST_F(AcceleratorServiceUnitTest, DuplicateDefaultsAreIgnored) {
  commands::AcceleratorService service(
      profile().GetPrefs(),
      {{IDC_FOCUS_MENU_BAR,
        {commands::FromCodesString("Alt"), commands::FromCodesString("Alt"),
         commands::FromCodesString("AltGr")}}},
      {});
  auto accelerators = service.GetAcceleratorsForTesting();
  ASSERT_EQ(2u, accelerators[IDC_FOCUS_MENU_BAR].size());
  EXPECT_EQ("Alt",
            commands::ToCodesString(accelerators[IDC_FOCUS_MENU_BAR][0]));
  EXPECT_EQ("AltGr",
            commands::ToCodesString(accelerators[IDC_FOCUS_MENU_BAR][1]));

  // Check that the modified flag is false - it has the same shortcuts as the
  // default even though the default has two Alt accelerators.
  auto command = service.GetCommandForTesting(IDC_FOCUS_MENU_BAR);
  EXPECT_EQ(2u, command->accelerators.size());
  EXPECT_FALSE(command->modified);

  // Add a new accelerator - we should detect the command was modified.
  service.AssignAcceleratorToCommand(IDC_FOCUS_MENU_BAR, "F6");
  command = service.GetCommandForTesting(IDC_FOCUS_MENU_BAR);
  EXPECT_EQ(3u, command->accelerators.size());
  EXPECT_TRUE(command->modified);

  // Resetting should remove the new accelerator and the modified flag should be
  // false again.
  service.ResetAcceleratorsForCommand(IDC_FOCUS_MENU_BAR);
  command = service.GetCommandForTesting(IDC_FOCUS_MENU_BAR);
  EXPECT_EQ(2u, command->accelerators.size());
  EXPECT_FALSE(command->modified);

  // If we delete one of the Alt accelerators the command should be marked as
  // modified.
  service.UnassignAcceleratorFromCommand(IDC_FOCUS_MENU_BAR, "Alt");
  command = service.GetCommandForTesting(IDC_FOCUS_MENU_BAR);
  EXPECT_EQ(1u, command->accelerators.size());
  EXPECT_TRUE(command->modified);

  // Resetting should add back the Alt accelerator.
  service.ResetAcceleratorsForCommand(IDC_FOCUS_MENU_BAR);
  command = service.GetCommandForTesting(IDC_FOCUS_MENU_BAR);
  EXPECT_EQ(2u, command->accelerators.size());
  EXPECT_FALSE(command->modified);
}

TEST_F(AcceleratorServiceUnitTest, UnmodifiableDefaultsAreReset) {
  commands::Accelerators defaults = {
      {IDC_FOCUS_MENU_BAR, {commands::FromCodesString("Alt+KeyF")}},
      {IDC_NEW_TAB, {commands::FromCodesString("Control+KeyT")}}};

  // First, move the default shortcut Ctrl+T to IDC_FOCUS_MENU_BAR
  {
    commands::AcceleratorService service(profile().GetPrefs(), defaults, {});

    // In future, this will be unmodifiable.
    service.AssignAcceleratorToCommand(IDC_FOCUS_MENU_BAR, "Control+KeyT");

    // Another shortcut, to check it isn't affected.
    service.AssignAcceleratorToCommand(IDC_NEW_TAB, "Control+KeyK");
  }

  // Then, relaunch the service with that as an unmodifiable shortcut.
  {
    commands::AcceleratorService service(
        profile().GetPrefs(), defaults,
        {commands::FromCodesString("Control+KeyT")});

    const auto& menu_command = service.GetCommandForTesting(IDC_FOCUS_MENU_BAR);
    ASSERT_EQ(1u, menu_command->accelerators.size());
    EXPECT_EQ("Alt+KeyF", menu_command->accelerators[0]->codes);

    const auto& nt_command = service.GetCommandForTesting(IDC_NEW_TAB);
    ASSERT_EQ(2u, nt_command->accelerators.size());
    EXPECT_EQ("Control+KeyK", nt_command->accelerators[0]->codes);
    const auto& unmodifiable_accelerator = nt_command->accelerators[1];
    EXPECT_TRUE(unmodifiable_accelerator->unmodifiable);
    EXPECT_EQ("Control+KeyT", unmodifiable_accelerator->codes);
  }
}

TEST_F(AcceleratorServiceUnitTest, PolicyFiltering) {
  commands::AcceleratorService service(profile().GetPrefs(), {}, {});

  // Test Brave News
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_CONFIGURE_BRAVE_NEWS));
  profile().GetPrefs()->SetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, true);
  EXPECT_TRUE(service.IsCommandDisabledByPolicy(IDC_CONFIGURE_BRAVE_NEWS));
  profile().GetPrefs()->SetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, false);
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_CONFIGURE_BRAVE_NEWS));

  // Test Brave Talk
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_SHOW_BRAVE_TALK));
  profile().GetPrefs()->SetBoolean(kBraveTalkDisabledByPolicy, true);
  EXPECT_TRUE(service.IsCommandDisabledByPolicy(IDC_SHOW_BRAVE_TALK));
  profile().GetPrefs()->SetBoolean(kBraveTalkDisabledByPolicy, false);
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_SHOW_BRAVE_TALK));

  // Test Brave VPN (multiple commands)
  const std::vector<int> vpn_commands = {IDC_SHOW_BRAVE_VPN_PANEL,
                                         IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON,
                                         IDC_TOGGLE_BRAVE_VPN_TRAY_ICON,
                                         IDC_SEND_BRAVE_VPN_FEEDBACK,
                                         IDC_ABOUT_BRAVE_VPN,
                                         IDC_MANAGE_BRAVE_VPN_PLAN,
                                         IDC_TOGGLE_BRAVE_VPN};
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  for (int command : vpn_commands) {
    EXPECT_FALSE(service.IsCommandDisabledByPolicy(command));
  }
  profile().GetPrefs()->SetBoolean(brave_vpn::prefs::kManagedBraveVPNDisabled,
                                   true);
  for (int command : vpn_commands) {
    EXPECT_TRUE(service.IsCommandDisabledByPolicy(command));
  }
  profile().GetPrefs()->SetBoolean(brave_vpn::prefs::kManagedBraveVPNDisabled,
                                   false);
#else
  // VPN not compiled in, should always return true (disabled)
  for (int command : vpn_commands) {
    EXPECT_TRUE(service.IsCommandDisabledByPolicy(command));
  }
#endif

  // Test Brave Wallet (multiple commands)
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  const std::vector<int> wallet_commands = {IDC_SHOW_BRAVE_WALLET,
                                            IDC_SHOW_BRAVE_WALLET_PANEL,
                                            IDC_CLOSE_BRAVE_WALLET_PANEL};
  for (int command : wallet_commands) {
    EXPECT_FALSE(service.IsCommandDisabledByPolicy(command));
  }
  profile().GetPrefs()->SetBoolean(brave_wallet::kBraveWalletDisabledByPolicy,
                                   true);
  for (int command : wallet_commands) {
    EXPECT_TRUE(service.IsCommandDisabledByPolicy(command));
  }
  profile().GetPrefs()->SetBoolean(brave_wallet::kBraveWalletDisabledByPolicy,
                                   false);
#endif

  // Test Brave Rewards
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_SHOW_BRAVE_REWARDS));
  profile().GetPrefs()->SetBoolean(brave_rewards::prefs::kDisabledByPolicy,
                                   true);
  EXPECT_TRUE(service.IsCommandDisabledByPolicy(IDC_SHOW_BRAVE_REWARDS));
  profile().GetPrefs()->SetBoolean(brave_rewards::prefs::kDisabledByPolicy,
                                   false);
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_SHOW_BRAVE_REWARDS));

#if BUILDFLAG(ENABLE_AI_CHAT)
  // Test AI Chat (reverse logic - disabled when pref is false)
  const std::vector<int> ai_chat_commands = {IDC_TOGGLE_AI_CHAT,
                                             IDC_OPEN_FULL_PAGE_CHAT};
  // Set AI Chat to disabled first (pref defaults may vary in test environment)
  profile().GetPrefs()->SetBoolean(ai_chat::prefs::kEnabledByPolicy, false);
  for (int command : ai_chat_commands) {
    EXPECT_TRUE(
        service.IsCommandDisabledByPolicy(command));  // Should be disabled
  }
  profile().GetPrefs()->SetBoolean(ai_chat::prefs::kEnabledByPolicy, true);
  for (int command : ai_chat_commands) {
    EXPECT_FALSE(service.IsCommandDisabledByPolicy(command));
  }
  profile().GetPrefs()->SetBoolean(ai_chat::prefs::kEnabledByPolicy, false);
  for (int command : ai_chat_commands) {
    EXPECT_TRUE(service.IsCommandDisabledByPolicy(command));
  }
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Test Speedreader
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_SPEEDREADER_ICON_ONCLICK));
  profile().GetPrefs()->SetBoolean(speedreader::kSpeedreaderEnabled, false);
  EXPECT_TRUE(service.IsCommandDisabledByPolicy(IDC_SPEEDREADER_ICON_ONCLICK));
  profile().GetPrefs()->SetBoolean(speedreader::kSpeedreaderEnabled, true);
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_SPEEDREADER_ICON_ONCLICK));
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  // Test Wayback Machine
  EXPECT_FALSE(
      service.IsCommandDisabledByPolicy(IDC_SHOW_WAYBACK_MACHINE_BUBBLE));
  profile().GetPrefs()->SetBoolean(kBraveWaybackMachineEnabled, false);
  EXPECT_TRUE(
      service.IsCommandDisabledByPolicy(IDC_SHOW_WAYBACK_MACHINE_BUBBLE));
  profile().GetPrefs()->SetBoolean(kBraveWaybackMachineEnabled, true);
  EXPECT_FALSE(
      service.IsCommandDisabledByPolicy(IDC_SHOW_WAYBACK_MACHINE_BUBBLE));
#endif

  // Test unknown commands (should not be disabled by policy)
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(IDC_NEW_TAB));
  EXPECT_FALSE(service.IsCommandDisabledByPolicy(99999));

  // Test FilterCommandsByPolicy
  commands::Accelerators test_accelerators = {
      {IDC_NEW_TAB, {commands::FromCodesString("Control+KeyT")}},
      {IDC_CONFIGURE_BRAVE_NEWS, {commands::FromCodesString("Control+KeyN")}},
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
      {IDC_SHOW_BRAVE_WALLET, {commands::FromCodesString("Control+KeyW")}},
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
      {IDC_TOGGLE_AI_CHAT, {commands::FromCodesString("Control+KeyC")}},
#endif
  };

  // Disable some features and test filtering
  profile().GetPrefs()->SetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, true);
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  profile().GetPrefs()->SetBoolean(brave_wallet::kBraveWalletDisabledByPolicy,
                                   true);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  profile().GetPrefs()->SetBoolean(ai_chat::prefs::kEnabledByPolicy,
                                   false);  // Disable AI Chat
#endif

  auto filtered = service.FilterCommandsByPolicy(test_accelerators);
#if BUILDFLAG(ENABLE_AI_CHAT)
  // Only IDC_NEW_TAB should remain (not policy-controlled)
  EXPECT_EQ(1u, filtered.size());
#else
  // IDC_NEW_TAB should remain (AI Chat not included)
  EXPECT_EQ(1u, filtered.size());
#endif
  EXPECT_TRUE(filtered.contains(IDC_NEW_TAB));
  EXPECT_FALSE(filtered.contains(IDC_CONFIGURE_BRAVE_NEWS));
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  EXPECT_FALSE(filtered.contains(IDC_SHOW_BRAVE_WALLET));
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  EXPECT_FALSE(filtered.contains(IDC_TOGGLE_AI_CHAT));
#endif

  // Re-enable commands and test again
  profile().GetPrefs()->SetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, false);
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  profile().GetPrefs()->SetBoolean(brave_wallet::kBraveWalletDisabledByPolicy,
                                   false);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  profile().GetPrefs()->SetBoolean(ai_chat::prefs::kEnabledByPolicy, true);
#endif

  filtered = service.FilterCommandsByPolicy(test_accelerators);
  // All commands should be present now
#if BUILDFLAG(ENABLE_AI_CHAT) && BUILDFLAG(ENABLE_BRAVE_WALLET)
  EXPECT_EQ(4u, filtered.size());
#elif BUILDFLAG(ENABLE_AI_CHAT) || BUILDFLAG(ENABLE_BRAVE_WALLET)
  EXPECT_EQ(3u, filtered.size());
#else
  EXPECT_EQ(2u, filtered.size());
#endif
  EXPECT_TRUE(filtered.contains(IDC_NEW_TAB));
  EXPECT_TRUE(filtered.contains(IDC_CONFIGURE_BRAVE_NEWS));
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  EXPECT_TRUE(filtered.contains(IDC_SHOW_BRAVE_WALLET));
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  EXPECT_TRUE(filtered.contains(IDC_TOGGLE_AI_CHAT));
#endif
}

class AcceleratorServiceUnitTestWithLocalState : public testing::Test {
 public:
  AcceleratorServiceUnitTestWithLocalState() {
    features_.InitAndEnableFeature(commands::features::kBraveCommands);
  }

  ~AcceleratorServiceUnitTestWithLocalState() override = default;

  TestingProfile& profile() { return profile_; }
  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->GetTestingLocalState();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  base::test::ScopedFeatureList features_;
};

TEST_F(AcceleratorServiceUnitTestWithLocalState, PolicyFiltering) {
  commands::AcceleratorService service(profile().GetPrefs(), {}, {});

#if BUILDFLAG(ENABLE_TOR)
  // Test Tor-related commands (which use local state)
  const std::vector<int> tor_commands = {IDC_NEW_OFFTHERECORD_WINDOW_TOR,
                                         IDC_NEW_TOR_CONNECTION_FOR_SITE};

  // Initially, commands should not be disabled
  for (int command : tor_commands) {
    EXPECT_FALSE(service.IsCommandDisabledByPolicy(command));
  }

  // Disable Tor via policy (using local state)
  local_state()->SetBoolean(tor::prefs::kTorDisabled, true);
  for (int command : tor_commands) {
    EXPECT_TRUE(service.IsCommandDisabledByPolicy(command));
  }

  // Re-enable Tor
  local_state()->SetBoolean(tor::prefs::kTorDisabled, false);
  for (int command : tor_commands) {
    EXPECT_FALSE(service.IsCommandDisabledByPolicy(command));
  }
#endif  // BUILDFLAG(ENABLE_TOR)
}

}  // namespace commands
