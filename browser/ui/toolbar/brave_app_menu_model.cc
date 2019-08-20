/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

BraveAppMenuModel::BraveAppMenuModel(
    ui::AcceleratorProvider* provider,
    Browser* browser,
    AppMenuIconController* app_menu_icon_controller)
    : AppMenuModel(provider, browser, app_menu_icon_controller),
      browser_(browser) {}

BraveAppMenuModel::~BraveAppMenuModel() {}

void BraveAppMenuModel::Build() {
  // Insert brave items after build chromium items.
  AppMenuModel::Build();
  InsertBraveMenuItems();
}

void BraveAppMenuModel::InsertBraveMenuItems() {
  // Sync & Rewards pages are redirected to normal window when it is loaded in
  // private window. So, only hide them in guest(tor) window.
  if (!browser_->profile()->IsGuestSession()) {
    bool walletEnabled = browser_->profile()->GetPrefs()->
        GetBoolean(kBraveWalletEnabled);
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_SHOW_DOWNLOADS),
        IDC_SHOW_BRAVE_REWARDS,
        IDS_SHOW_BRAVE_REWARDS);
    if (walletEnabled) {
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
      InsertItemWithStringIdAt(GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS),
                               IDC_SHOW_BRAVE_WALLET, IDS_SHOW_BRAVE_WALLET);
#endif
    }
    InsertItemWithStringIdAt(
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
        GetIndexOfCommandId(walletEnabled ? IDC_SHOW_BRAVE_WALLET :
            IDC_SHOW_BRAVE_REWARDS),
#else
        GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS),
#endif
        IDC_SHOW_BRAVE_SYNC, IDS_SHOW_BRAVE_SYNC);
  }

  // Insert Create New Profile item
  auto profile_items_insert_index = GetIndexOfCommandId(IDC_ZOOM_MENU) - 1;
  InsertItemWithStringIdAt(profile_items_insert_index,
    IDC_OPEN_GUEST_PROFILE,
    IDS_OPEN_GUEST_PROFILE);
  InsertItemWithStringIdAt(
    GetIndexOfCommandId(IDC_OPEN_GUEST_PROFILE),
    IDC_ADD_NEW_PROFILE,
    IDS_ADD_NEW_PROFILE);
  InsertSeparatorAt(
    GetIndexOfCommandId(IDC_ADD_NEW_PROFILE), ui::NORMAL_SEPARATOR);

  InsertItemWithStringIdAt(
      GetIndexOfCommandId(IDC_SHOW_DOWNLOADS),
      IDC_SHOW_BRAVE_ADBLOCK,
      IDS_SHOW_BRAVE_ADBLOCK);
  if (brave::IsTorProfile(browser_->profile())) {
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_NEW_WINDOW),
        IDC_NEW_TOR_CONNECTION_FOR_SITE,
        IDS_NEW_TOR_CONNECTION_FOR_SITE);
  } else {
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_NEW_INCOGNITO_WINDOW) + 1,
        IDC_NEW_OFFTHERECORD_WINDOW_TOR,
        IDS_NEW_OFFTHERECORD_WINDOW_TOR);
  }
}
