/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"

BraveAppMenuModel::BraveAppMenuModel(
    ui::AcceleratorProvider* provider, Browser* browser)
    : AppMenuModel(provider, browser),
      browser_(browser) {}

BraveAppMenuModel::~BraveAppMenuModel() {}

void BraveAppMenuModel::Build() {
  // Insert brave items after build chromium items.
  AppMenuModel::Build();
  InsertBraveMenuItems();
}

void BraveAppMenuModel::InsertBraveMenuItems() {
  InsertItemWithStringIdAt(
      GetIndexOfCommandId(IDC_SHOW_DOWNLOADS),
      IDC_SHOW_BRAVE_REWARDS,
      IDS_SHOW_BRAVE_REWARDS);
  InsertItemWithStringIdAt(
      GetIndexOfCommandId(IDC_SHOW_DOWNLOADS),
      IDC_SHOW_BRAVE_ADBLOCK,
      IDS_SHOW_BRAVE_ADBLOCK);
  if (browser_->profile()->IsTorProfile()) {
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_NEW_WINDOW),
        IDC_NEW_TOR_IDENTITY,
        IDS_NEW_TOR_IDENTITY);
   }
  InsertItemWithStringIdAt(
      GetIndexOfCommandId(IDC_SHOW_BRAVE_REWARDS),
      IDC_SHOW_BRAVE_SYNC,
      IDS_SHOW_BRAVE_SYNC);
}
