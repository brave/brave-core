/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_command_controller.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace {
bool IsBraveCommands(int id) {
  return id >= IDC_BRAVE_COMMANDS_START && id <= IDC_BRAVE_COMMANDS_LAST;
}
}

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
    int id, WindowOpenDisposition disposition) {
  return IsBraveCommands(id)
      ? ExecuteBraveCommandWithDisposition(id, disposition)
      : BrowserCommandController::ExecuteCommandWithDisposition(id,
                                                                disposition);
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
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  UpdateCommandForBraveRewards();
#endif
  UpdateCommandForBraveAdblock();
  UpdateCommandForTor();
  UpdateCommandForBraveSync();
}

void BraveBrowserCommandController::UpdateCommandForBraveRewards() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_REWARDS, true);
}

void BraveBrowserCommandController::UpdateCommandForBraveAdblock() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_ADBLOCK, true);
}

void BraveBrowserCommandController::UpdateCommandForTor() {
  UpdateCommandEnabled(IDC_NEW_TOR_IDENTITY, true);
}

void BraveBrowserCommandController::UpdateCommandForBraveSync() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_SYNC, true);
}

bool BraveBrowserCommandController::ExecuteBraveCommandWithDisposition(
    int id, WindowOpenDisposition disposition) {
  if (!SupportsCommand(id) || !IsCommandEnabled(id))
    return false;

  if (browser_->tab_strip_model()->active_index() == TabStripModel::kNoTab)
    return true;

  DCHECK(brave_command_updater_.IsCommandEnabled(id))
      << "Invalid/disabled command " << id;

  switch (id) {
    case IDC_SHOW_BRAVE_REWARDS:
      brave::ShowBraveRewards(browser_);
      break;
    case IDC_SHOW_BRAVE_ADBLOCK:
      brave::ShowBraveAdblock(browser_);
      break;
    case IDC_NEW_TOR_IDENTITY:
      brave::NewTorIdentity(browser_);
      break;
    case IDC_SHOW_BRAVE_SYNC:
      brave::ShowBraveSync(browser_);
      break;

    default:
      LOG(WARNING) << "Received Unimplemented Command: " << id;
      break;
  }

  return true;
}

}  // namespace chrome
