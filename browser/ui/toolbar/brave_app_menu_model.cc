/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"

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
  InsertItemWithStringIdAt(
      GetIndexOfCommandId(IDC_SHOW_DOWNLOADS),
      IDC_SHOW_BRAVE_ADS,
      IDC_SHOW_BRAVE_ADS);
}
