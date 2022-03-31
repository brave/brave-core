/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_command_controller.h"

#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/notreached.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/features.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/command_line_switches.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/browser/ui/sidebar/sidebar_utils.h"
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
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  Observe(BraveVpnServiceFactory::GetForProfile(browser_->profile()));
#endif
}

BraveBrowserCommandController::~BraveBrowserCommandController() = default;

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
    UpdateCommandForBraveRewards();
    UpdateCommandForBraveWallet();
    if (syncer::IsSyncAllowedByFlag())
      UpdateCommandForBraveSync();
  }
  UpdateCommandForBraveAdblock();
  UpdateCommandForWebcompatReporter();
#if BUILDFLAG(ENABLE_TOR)
  UpdateCommandForTor();
#endif
  UpdateCommandForSidebar();
  UpdateCommandForBraveVPN();
  bool add_new_profile_enabled = !is_guest_session;
  bool open_guest_profile_enabled = !is_guest_session;
  if (!is_guest_session) {
    if (PrefService* local_state = g_browser_process->local_state()) {
      add_new_profile_enabled =
          local_state->GetBoolean(prefs::kBrowserAddPersonEnabled);
      open_guest_profile_enabled =
          local_state->GetBoolean(prefs::kBrowserGuestModeEnabled);
    }
  }
  UpdateCommandEnabled(IDC_ADD_NEW_PROFILE, add_new_profile_enabled);
  UpdateCommandEnabled(IDC_OPEN_GUEST_PROFILE, open_guest_profile_enabled);

  if (base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    UpdateCommandEnabled(IDC_SPEEDREADER_ICON_ONCLICK, true);
    UpdateCommandEnabled(IDC_DISTILL_PAGE, false);
  }
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  UpdateCommandEnabled(IDC_APP_MENU_IPFS_OPEN_FILES, true);
#endif
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
                       browser_->profile()->IsTor());
  UpdateCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR,
                       !brave::IsTorDisabledForProfile(browser_->profile()));
}
#endif

void BraveBrowserCommandController::UpdateCommandForSidebar() {
#if BUILDFLAG(ENABLE_SIDEBAR)
  if (sidebar::CanUseSidebar(browser_))
    UpdateCommandEnabled(IDC_SIDEBAR_SHOW_OPTION_MENU, true);
#endif
}

void BraveBrowserCommandController::UpdateCommandForBraveVPN() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (!brave_vpn::IsBraveVPNEnabled()) {
    UpdateCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL, false);
    UpdateCommandEnabled(IDC_BRAVE_VPN_MENU, false);
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON, false);
    UpdateCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK, false);
    UpdateCommandEnabled(IDC_ABOUT_BRAVE_VPN, false);
    UpdateCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN, false);
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN, false);
    return;
  }

  UpdateCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL, true);
  UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON, true);
  UpdateCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK, true);
  UpdateCommandEnabled(IDC_ABOUT_BRAVE_VPN, true);
  UpdateCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN, true);

  if (auto* vpn_service =
          BraveVpnServiceFactory::GetForProfile(browser_->profile())) {
    // Only show vpn sub menu for purchased user.
    UpdateCommandEnabled(IDC_BRAVE_VPN_MENU, vpn_service->is_purchased_user());
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN,
                         vpn_service->is_purchased_user());
  }
#endif
}

void BraveBrowserCommandController::UpdateCommandForBraveSync() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_SYNC, true);
}

void BraveBrowserCommandController::UpdateCommandForBraveWallet() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_WALLET, true);
  UpdateCommandEnabled(IDC_SHOW_BRAVE_WALLET_PANEL, true);
  UpdateCommandEnabled(IDC_CLOSE_BRAVE_WALLET_PANEL, true);
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
      if (!browser_->profile()->IsTor())
        return BrowserCommandController::ExecuteCommandWithDisposition(
            id, disposition, time_stamp);
      NewEmptyWindow(browser_->profile()->GetOriginalProfile());
      break;
    case IDC_NEW_INCOGNITO_WINDOW:
      // Use chromium's action for non-Tor profiles.
      if (!browser_->profile()->IsTor())
        return BrowserCommandController::ExecuteCommandWithDisposition(
            id, disposition, time_stamp);
      NewIncognitoWindow(browser_->profile()->GetOriginalProfile());
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
      brave::ShowSync(browser_);
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
    case IDC_SPEEDREADER_ICON_ONCLICK:
      brave::MaybeDistillAndShowSpeedreaderBubble(browser_);
      break;
    case IDC_SHOW_BRAVE_WALLET_PANEL:
      brave::ShowWalletBubble(browser_);
      break;
    case IDC_CLOSE_BRAVE_WALLET_PANEL:
      brave::CloseWalletBubble(browser_);
      break;
    case IDC_SHOW_BRAVE_VPN_PANEL:
      brave::ShowBraveVPNBubble(browser_);
      break;
    case IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON:
      brave::ToggleBraveVPNButton(browser_);
      break;
    case IDC_SEND_BRAVE_VPN_FEEDBACK:
    case IDC_ABOUT_BRAVE_VPN:
    case IDC_MANAGE_BRAVE_VPN_PLAN:
      brave::OpenBraveVPNUrls(browser_, id);
      break;
    case IDC_APP_MENU_IPFS_OPEN_FILES:
      brave::OpenIpfsFilesWebUI(browser_);
      break;
    default:
      LOG(WARNING) << "Received Unimplemented Command: " << id;
      break;
  }

  return true;
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void BraveBrowserCommandController::OnPurchasedStateChanged(
    brave_vpn::mojom::PurchasedState state) {
  UpdateCommandForBraveVPN();
}
#endif

}  // namespace chrome
