/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
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

bool BraveAppMenuModel::ShouldShowNewIncognitoWindowMenuItem() {
  // The profile for private windows with Tor is a guest session,
  // which ordinarily suppresses the menu item to open new private
  // windows, but we don't want to suppress that here.
  if (browser_->profile()->IsTorProfile())
    return true;

  return AppMenuModel::ShouldShowNewIncognitoWindowMenuItem();
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
      GetIndexOfCommandId(IDC_NEW_INCOGNITO_WINDOW) + 1,
      IDC_NEW_OFFTHERECORD_WINDOW_TOR,
      IDS_NEW_OFFTHERECORD_WINDOW_TOR);
  if (browser_->profile()->IsTorProfile()) {
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_NEW_INCOGNITO_WINDOW) + 2,
        IDC_NEW_TOR_IDENTITY,
        IDS_NEW_TOR_IDENTITY);
   }
}
