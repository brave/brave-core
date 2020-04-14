/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_command_controller.h"

#include <vector>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "components/sync/driver/sync_driver_switches.h"
#endif

namespace {

bool IsBraveCommands(int id) {
  return id >= IDC_BRAVE_COMMANDS_START && id <= IDC_BRAVE_COMMANDS_LAST;
}

bool IsBraveOverrideCommands(int id) {
  static std::vector<int> override_commands({
      IDC_NEW_WINDOW,
      IDC_NEW_INCOGNITO_WINDOW,
  });
  return std::find(override_commands.begin(), override_commands.end(), id) !=
         override_commands.end();
}

}  // namespace

namespace chrome {

BraveBrowserCommandController::BraveBrowserCommandController(Browser* browser)
    : BrowserCommandController(browser),
      browser_(browser),
      brave_command_updater_(nullptr) {
  InitBraveCommandState();
}

bool BraveBrowserCommandController::SupportsCommand(int id) const {
  return IsBraveCommands(id)
      ? brave_command_updater_.SupportsCommand(id)
      : BrowserCommandController::SupportsCommand(id);
}

bool BraveBrowserCommandController::IsCommandEnabled(int id) const {
  return IsBraveCommands(id)
      ? brave_command_updater_.IsCommandEnabled(id)
      : BrowserCommandController::IsCommandEnabled(id);
}

bool BraveBrowserCommandController::ExecuteCommandWithDisposition(
    int id,
    WindowOpenDisposition disposition,
    base::TimeTicks time_stamp) {
  return IsBraveCommands(id) || IsBraveOverrideCommands(id)
             ? ExecuteBraveCommandWithDisposition(id, disposition, time_stamp)
             : BrowserCommandController::ExecuteCommandWithDisposition(
                   id, disposition, time_stamp);
}

void BraveBrowserCommandController::AddCommandObserver(
    int id, CommandObserver* observer) {
  IsBraveCommands(id)
      ? brave_command_updater_.AddCommandObserver(id, observer)
      : BrowserCommandController::AddCommandObserver(id, observer);
}

void BraveBrowserCommandController::RemoveCommandObserver(
    int id, CommandObserver* observer) {
  IsBraveCommands(id)
      ? brave_command_updater_.RemoveCommandObserver(id, observer)
      : BrowserCommandController::RemoveCommandObserver(id, observer);
}

void BraveBrowserCommandController::RemoveCommandObserver(
    CommandObserver* observer) {
  brave_command_updater_.RemoveCommandObserver(observer);
  BrowserCommandController::RemoveCommandObserver(observer);
}

bool BraveBrowserCommandController::UpdateCommandEnabled(int id, bool state) {
  return IsBraveCommands(id)
      ? brave_command_updater_.UpdateCommandEnabled(id, state)
      : BrowserCommandController::UpdateCommandEnabled(id, state);
}

void BraveBrowserCommandController::InitBraveCommandState() {
  // Sync & Rewards pages doesn't work on tor(guest) session.
  // They also doesn't work on private window but they are redirected
  // to normal window in this case.
  const bool is_guest_session = browser_->profile()->IsGuestSession();
  if (!is_guest_session) {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
    UpdateCommandForBraveRewards();
#endif
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
    UpdateCommandForBraveWallet();
#endif
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
    if (switches::IsSyncAllowedByFlag())
      UpdateCommandForBraveSync();
#endif
  }
  UpdateCommandForBraveAdblock();
  UpdateCommandForWebcompatReporter();
#if BUILDFLAG(ENABLE_TOR)
  UpdateCommandForTor();
#endif
  UpdateCommandEnabled(IDC_ADD_NEW_PROFILE, !is_guest_session);
  UpdateCommandEnabled(IDC_OPEN_GUEST_PROFILE, !is_guest_session);
  UpdateCommandEnabled(IDC_TOGGLE_SPEEDREADER, true);
}

void BraveBrowserCommandController::UpdateCommandForBraveRewards() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_REWARDS, true);
}

void BraveBrowserCommandController::UpdateCommandForBraveAdblock() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK, true);
}

void BraveBrowserCommandController::UpdateCommandForWebcompatReporter() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER, true);
}

#if BUILDFLAG(ENABLE_TOR)
void BraveBrowserCommandController::UpdateCommandForTor() {
  // Enable new tor connection only for tor profile.
  UpdateCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE,
                       brave::IsTorProfile(browser_->profile()));
  UpdateCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR,
                       !brave::IsTorDisabledForProfile(browser_->profile()));
}
#endif

void BraveBrowserCommandController::UpdateCommandForBraveSync() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_SYNC, true);
}

void BraveBrowserCommandController::UpdateCommandForBraveWallet() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_WALLET, true);
}

bool BraveBrowserCommandController::ExecuteBraveCommandWithDisposition(
    int id,
    WindowOpenDisposition disposition,
    base::TimeTicks time_stamp) {
  if (!SupportsCommand(id) || !IsCommandEnabled(id))
    return false;

  if (browser_->tab_strip_model()->active_index() == TabStripModel::kNoTab)
    return true;

  DCHECK(IsCommandEnabled(id)) << "Invalid/disabled command " << id;

  switch (id) {
    case IDC_NEW_WINDOW:
      // Use chromium's action for non-Tor profiles.
      if (!brave::IsTorProfile(browser_->profile()))
        return BrowserCommandController::ExecuteCommandWithDisposition(
            id, disposition, time_stamp);
      NewEmptyWindow(brave::GetParentProfile(browser_->profile()));
      break;
    case IDC_NEW_INCOGNITO_WINDOW:
      // Use chromium's action for non-Tor profiles.
      if (!brave::IsTorProfile(browser_->profile()))
        return BrowserCommandController::ExecuteCommandWithDisposition(
            id, disposition, time_stamp);
      NewIncognitoWindow(brave::GetParentProfile(browser_->profile()));
      break;
    case IDC_SHOW_BRAVE_REWARDS:
      brave::ShowBraveRewards(browser_);
      break;
    case IDC_SHOW_BRAVE_ADBLOCK:
      brave::ShowBraveAdblock(browser_);
      break;
    case IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER:
      brave::ShowWebcompatReporter(browser_);
      break;
    case IDC_NEW_OFFTHERECORD_WINDOW_TOR:
      brave::NewOffTheRecordWindowTor(browser_);
      break;
    case IDC_NEW_TOR_CONNECTION_FOR_SITE:
      brave::NewTorConnectionForSite(browser_);
      break;
    case IDC_SHOW_BRAVE_SYNC:
      brave::ShowBraveSync(browser_);
      break;
    case IDC_SHOW_BRAVE_WALLET:
      brave::ShowBraveWallet(browser_);
      break;
    case IDC_ADD_NEW_PROFILE:
      brave::AddNewProfile();
      break;
    case IDC_OPEN_GUEST_PROFILE:
      brave::OpenGuestProfile();
      break;
    case IDC_TOGGLE_SPEEDREADER:
      brave::ToggleSpeedreader(browser_);
      break;
    default:
      LOG(WARNING) << "Received Unimplemented Command: " << id;
      break;
  }

  return true;
}

}  // namespace chrome
