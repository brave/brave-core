/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"

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
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_SHOW_DOWNLOADS),
        IDC_SHOW_BRAVE_REWARDS,
        IDS_SHOW_BRAVE_REWARDS);
    if (brave_sync::BraveSyncService::is_enabled()) {
      InsertItemWithStringIdAt(
          GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS),
          IDC_SHOW_BRAVE_SYNC,
          IDS_SHOW_BRAVE_SYNC);
    }
  }
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
