/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_command_controller.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/debug/stack_trace.h"
#include "base/feature_list.h"
#include "base/notreached.h"
#include "base/types/to_address.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/commands/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/send_tab_to_self/send_tab_to_self_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_change_type.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/common/features.h"
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/browser/ui/commander/commander_service.h"
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
  return base::Contains(override_commands, id);
}

}  // namespace

namespace chrome {

BraveBrowserCommandController::BraveBrowserCommandController(Browser* browser)
    : BrowserCommandController(browser),
      browser_(*browser),
      brave_command_updater_(nullptr) {
  InitBraveCommandState();
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (auto* vpn_service = brave_vpn::BraveVpnServiceFactory::GetForProfile(
          browser_->profile())) {
    brave_vpn::BraveVPNServiceObserver::Observe(vpn_service);
  }
#endif
}

BraveBrowserCommandController::~BraveBrowserCommandController() = default;

void BraveBrowserCommandController::TabChangedAt(content::WebContents* contents,
                                                 int index,
                                                 TabChangeType type) {
  UpdateCommandEnabled(IDC_CLOSE_DUPLICATE_TABS,
                       brave::HasDuplicateTabs(&*browser_));
  UpdateCommandsForTabs();
  UpdateCommandsForSend();
}

void BraveBrowserCommandController::TabPinnedStateChanged(
    TabStripModel* tab_strip_model,
    content::WebContents* contents,
    int index) {
  UpdateCommandsForPin();
}

void BraveBrowserCommandController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  BrowserCommandController::OnTabStripModelChanged(tab_strip_model, change,
                                                   selection);

  UpdateCommandEnabled(IDC_WINDOW_CLOSE_TABS_TO_LEFT,
                       brave::CanCloseTabsToLeft(&*browser_));
  UpdateCommandEnabled(IDC_CLOSE_DUPLICATE_TABS,
                       brave::HasDuplicateTabs(&*browser_));
  UpdateCommandsForTabs();
  UpdateCommandsForSend();
  UpdateCommandsForPin();

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) &&
      browser_->is_type_normal() && selection.active_tab_changed()) {
    UpdateCommandForSplitView();
  }
}

void BraveBrowserCommandController::OnTabGroupChanged(
    const TabGroupChange& change) {
  UpdateCommandsForTabs();
}

void BraveBrowserCommandController::OnTileTabs(
    const SplitViewBrowserData::Tile& tile) {
  UpdateCommandForSplitView();
}

void BraveBrowserCommandController::OnWillBreakTile(
    const SplitViewBrowserData::Tile& tile) {
  UpdateCommandForSplitView();
}

void BraveBrowserCommandController::OnWillDeleteBrowserData() {
  split_view_browser_data_observation_.Reset();
}

bool BraveBrowserCommandController::SupportsCommand(int id) const {
  return IsBraveCommands(id) ? brave_command_updater_.SupportsCommand(id)
                             : BrowserCommandController::SupportsCommand(id);
}

bool BraveBrowserCommandController::IsCommandEnabled(int id) const {
  return IsBraveCommands(id) ? brave_command_updater_.IsCommandEnabled(id)
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
    int id,
    CommandObserver* observer) {
  IsBraveCommands(id)
      ? brave_command_updater_.AddCommandObserver(id, observer)
      : BrowserCommandController::AddCommandObserver(id, observer);
}

void BraveBrowserCommandController::RemoveCommandObserver(
    int id,
    CommandObserver* observer) {
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
  // Sync, Rewards, and Wallet pages don't work in tor(guest) sessions.
  // They also don't work in private windows but they are redirected
  // to a normal window in this case.
  const bool is_guest_session = browser_->profile()->IsGuestSession();
  if (!is_guest_session) {
    // If Rewards is not supported due to OFAC sanctions we still want to show
    // the menu item.
    if (brave_rewards::IsSupported(browser_->profile()->GetPrefs())) {
      UpdateCommandForBraveRewards();
    }
    if (brave_wallet::IsAllowed(browser_->profile()->GetPrefs())) {
      UpdateCommandForBraveWallet();
    }
    if (syncer::IsSyncAllowedByFlag()) {
      UpdateCommandForBraveSync();
    }
  }
  UpdateCommandForWebcompatReporter();
#if BUILDFLAG(ENABLE_TOR)
  UpdateCommandForTor();
#endif
  UpdateCommandForSidebar();
  UpdateCommandForBraveVPN();
  UpdateCommandForPlaylist();
  UpdateCommandForWaybackMachine();
#if BUILDFLAG(ENABLE_AI_CHAT) || BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_change_registrar_.Init(browser_->profile()->GetPrefs());
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  UpdateCommandForAIChat();
  if (ai_chat::IsAllowedForContext(browser_->profile(), false)) {
    pref_change_registrar_.Add(
        ai_chat::prefs::kEnabledByPolicy,
        base::BindRepeating(
            &BraveBrowserCommandController::UpdateCommandForAIChat,
            base::Unretained(this)));
  }
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsAllowedForContext(browser_->profile())) {
    pref_change_registrar_.Add(
        brave_vpn::prefs::kManagedBraveVPNDisabled,
        base::BindRepeating(
            &BraveBrowserCommandController::UpdateCommandForBraveVPN,
            base::Unretained(this)));
  }
#endif
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
  UpdateCommandEnabled(IDC_COPY_CLEAN_LINK, true);
  UpdateCommandEnabled(IDC_TOGGLE_TAB_MUTE, true);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  if (base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    UpdateCommandEnabled(IDC_SPEEDREADER_ICON_ONCLICK, true);
  }
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
  UpdateCommandEnabled(IDC_COMMANDER, commander::IsEnabled());
#endif

  UpdateCommandEnabled(IDC_BRAVE_BOOKMARK_BAR_SUBMENU, true);

  UpdateCommandEnabled(IDC_TOGGLE_VERTICAL_TABS, true);
  UpdateCommandEnabled(IDC_TOGGLE_VERTICAL_TABS_WINDOW_TITLE, true);
  UpdateCommandEnabled(IDC_TOGGLE_VERTICAL_TABS_EXPANDED, true);

  UpdateCommandEnabled(IDC_CONFIGURE_BRAVE_NEWS,
                       !browser_->profile()->IsOffTheRecord());

  UpdateCommandEnabled(
      IDC_CONFIGURE_SHORTCUTS,
      base::FeatureList::IsEnabled(commands::features::kBraveCommands));

  UpdateCommandEnabled(IDC_SHOW_BRAVE_TALK, true);
  UpdateCommandEnabled(IDC_TOGGLE_SHIELDS, true);
  UpdateCommandEnabled(IDC_TOGGLE_JAVASCRIPT, true);

  UpdateCommandEnabled(IDC_CLOSE_DUPLICATE_TABS,
                       brave::HasDuplicateTabs(&*browser_));
  UpdateCommandEnabled(IDC_WINDOW_ADD_ALL_TABS_TO_NEW_GROUP, true);

  UpdateCommandEnabled(IDC_SCROLL_TAB_TO_TOP, true);
  UpdateCommandEnabled(IDC_SCROLL_TAB_TO_BOTTOM, true);

  UpdateCommandEnabled(IDC_BRAVE_SEND_TAB_TO_SELF, true);

  UpdateCommandsForTabs();
  UpdateCommandsForSend();
  UpdateCommandsForPin();

  UpdateCommandEnabled(IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY, true);

  if (browser_->is_type_normal()) {
    // Delete these when upstream enables by default.
    UpdateCommandEnabled(IDC_READING_LIST_MENU, true);
    UpdateCommandEnabled(IDC_READING_LIST_MENU_ADD_TAB, true);
    UpdateCommandEnabled(IDC_READING_LIST_MENU_SHOW_UI, true);
  }

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) &&
      browser_->is_type_normal()) {
    UpdateCommandForSplitView();
  }
}

void BraveBrowserCommandController::UpdateCommandForBraveRewards() {
  UpdateCommandEnabled(IDC_SHOW_BRAVE_REWARDS, true);
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
  if (sidebar::CanUseSidebar(&*browser_)) {
    UpdateCommandEnabled(IDC_SIDEBAR_SHOW_OPTION_MENU, true);
    UpdateCommandEnabled(IDC_SIDEBAR_TOGGLE_POSITION, true);
    UpdateCommandEnabled(IDC_TOGGLE_SIDEBAR, true);
  }
}

#if BUILDFLAG(ENABLE_AI_CHAT)
void BraveBrowserCommandController::UpdateCommandForAIChat() {
  // AI Chat command implementation needs sidebar
  bool command_enabled = (sidebar::CanUseSidebar(&*browser_) &&
                          ai_chat::IsAllowedForContext(browser_->profile()));
  UpdateCommandEnabled(IDC_TOGGLE_AI_CHAT, command_enabled);
}
#endif

void BraveBrowserCommandController::UpdateCommandForBraveVPN() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (!brave_vpn::IsBraveVPNEnabled(browser_->profile())) {
    UpdateCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL, false);
    UpdateCommandEnabled(IDC_BRAVE_VPN_MENU, false);
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON, false);
    UpdateCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK, false);
    UpdateCommandEnabled(IDC_ABOUT_BRAVE_VPN, false);
    UpdateCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN, false);
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN, false);
#if BUILDFLAG(IS_WIN)
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN_TRAY_ICON, false);
#endif
    return;
  }
  UpdateCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL, true);
  UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON, true);
#if BUILDFLAG(IS_WIN)
  UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN_TRAY_ICON, true);
#endif
  UpdateCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK, true);
  UpdateCommandEnabled(IDC_ABOUT_BRAVE_VPN, true);
  UpdateCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN, true);

  if (auto* vpn_service = brave_vpn::BraveVpnServiceFactory::GetForProfile(
          browser_->profile())) {
    // Only show vpn sub menu for purchased user.
    UpdateCommandEnabled(IDC_BRAVE_VPN_MENU, vpn_service->is_purchased_user());
    UpdateCommandEnabled(IDC_TOGGLE_BRAVE_VPN,
                         vpn_service->is_purchased_user());
  }
#endif
}

void BraveBrowserCommandController::UpdateCommandForPlaylist() {
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    UpdateCommandEnabled(
        IDC_SHOW_PLAYLIST_BUBBLE,
        browser_->is_type_normal() &&
            playlist::PlaylistServiceFactory::GetForBrowserContext(
                browser_->profile()));
  }
#endif
}

void BraveBrowserCommandController::UpdateCommandForWaybackMachine() {
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  UpdateCommandEnabled(IDC_SHOW_WAYBACK_MACHINE_BUBBLE, true);
#endif
}

void BraveBrowserCommandController::UpdateCommandsForTabs() {
  UpdateCommandEnabled(IDC_WINDOW_MUTE_ALL_TABS,
                       brave::CanMuteAllTabs(&*browser_, false));
  UpdateCommandEnabled(IDC_WINDOW_MUTE_OTHER_TABS,
                       brave::CanMuteAllTabs(&*browser_, true));
  UpdateCommandEnabled(IDC_WINDOW_UNMUTE_ALL_TABS,
                       brave::CanUnmuteAllTabs(&*browser_));

  UpdateCommandEnabled(IDC_GROUP_TABS_ON_CURRENT_ORIGIN, true);
  UpdateCommandEnabled(IDC_MOVE_GROUP_TO_NEW_WINDOW, true);

  bool is_in_group = brave::IsInGroup(&*browser_);
  bool has_ungrouped_tabs = brave::HasUngroupedTabs(&*browser_);
  UpdateCommandEnabled(IDC_WINDOW_GROUP_UNGROUPED_TABS, has_ungrouped_tabs);
  UpdateCommandEnabled(IDC_WINDOW_UNGROUP_GROUP, is_in_group);
  UpdateCommandEnabled(IDC_WINDOW_REMOVE_TAB_FROM_GROUP, is_in_group);
  UpdateCommandEnabled(IDC_WINDOW_NAME_GROUP, is_in_group);
  UpdateCommandEnabled(IDC_WINDOW_NEW_TAB_IN_GROUP, is_in_group);
  UpdateCommandEnabled(IDC_WINDOW_UNGROUP_ALL_TABS,
                       brave::CanUngroupAllTabs(&*browser_));
  UpdateCommandEnabled(IDC_WINDOW_TOGGLE_GROUP_EXPANDED, is_in_group);
  UpdateCommandEnabled(IDC_WINDOW_CLOSE_UNGROUPED_TABS, has_ungrouped_tabs);
  UpdateCommandEnabled(IDC_WINDOW_CLOSE_TABS_NOT_IN_CURRENT_GROUP, is_in_group);
  UpdateCommandEnabled(IDC_WINDOW_CLOSE_GROUP, is_in_group);

  UpdateCommandEnabled(IDC_WINDOW_BRING_ALL_TABS,
                       brave::CanBringAllTabs(&*browser_));
}

void BraveBrowserCommandController::UpdateCommandsForSend() {
  UpdateCommandEnabled(
      IDC_BRAVE_SEND_TAB_TO_SELF,
      send_tab_to_self::ShouldDisplayEntryPoint(
          browser_->tab_strip_model()->GetActiveWebContents()));
}

void BraveBrowserCommandController::UpdateCommandsForPin() {
  UpdateCommandEnabled(IDC_WINDOW_CLOSE_UNPINNED_TABS,
                       brave::CanCloseUnpinnedTabs(&*browser_));
}

void BraveBrowserCommandController::UpdateCommandForSplitView() {
  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(base::to_address(browser_));
  if (!split_view_browser_data) {
    // Can happen on start up.
    return;
  }

  if (!split_view_browser_data_observation_.IsObserving()) {
    split_view_browser_data_observation_.Observe(split_view_browser_data);
  }

  UpdateCommandEnabled(IDC_NEW_SPLIT_VIEW, brave::CanOpenNewSplitViewForTab(
                                               base::to_address(browser_)));
  UpdateCommandEnabled(IDC_TILE_TABS,
                       brave::CanTileTabs(base::to_address(browser_)));

  const auto is_tab_tiled = brave::IsTabsTiled(base::to_address(browser_));
  for (auto command_enabled_when_tab_is_tiled :
       {IDC_BREAK_TILE, IDC_SWAP_SPLIT_VIEW}) {
    UpdateCommandEnabled(command_enabled_when_tab_is_tiled, is_tab_tiled);
  }
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
  if (!SupportsCommand(id) || !IsCommandEnabled(id)) {
    return false;
  }

  if (browser_->tab_strip_model()->active_index() == TabStripModel::kNoTab) {
    return true;
  }

  DCHECK(IsCommandEnabled(id)) << "Invalid/disabled command " << id;

  switch (id) {
    case IDC_NEW_WINDOW:
      // Use chromium's action for non-Tor profiles.
      if (!browser_->profile()->IsTor()) {
        return BrowserCommandController::ExecuteCommandWithDisposition(
            id, disposition, time_stamp);
      }
      NewEmptyWindow(browser_->profile()->GetOriginalProfile());
      break;
    case IDC_NEW_INCOGNITO_WINDOW:
      // Use chromium's action for non-Tor profiles.
      if (!browser_->profile()->IsTor()) {
        return BrowserCommandController::ExecuteCommandWithDisposition(
            id, disposition, time_stamp);
      }
      NewIncognitoWindow(browser_->profile()->GetOriginalProfile());
      break;
    case IDC_SHOW_BRAVE_REWARDS:
      brave::ShowBraveRewards(&*browser_);
      break;
    case IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER:
      brave::ShowWebcompatReporter(&*browser_);
      break;
    case IDC_NEW_OFFTHERECORD_WINDOW_TOR:
      brave::NewOffTheRecordWindowTor(&*browser_);
      break;
    case IDC_NEW_TOR_CONNECTION_FOR_SITE:
      brave::NewTorConnectionForSite(&*browser_);
      break;
    case IDC_SHOW_BRAVE_SYNC:
      brave::ShowSync(&*browser_);
      break;
    case IDC_SHOW_BRAVE_WALLET:
      brave::ShowBraveWallet(&*browser_);
      break;
    case IDC_TOGGLE_AI_CHAT:
#if BUILDFLAG(ENABLE_AI_CHAT)
      brave::ToggleAIChat(&*browser_);
#endif
      break;
    case IDC_SPEEDREADER_ICON_ONCLICK:
      brave::MaybeDistillAndShowSpeedreaderBubble(&*browser_);
      break;
    case IDC_SHOW_BRAVE_WALLET_PANEL:
      brave::ShowWalletBubble(&*browser_);
      break;
    case IDC_CLOSE_BRAVE_WALLET_PANEL:
      brave::CloseWalletBubble(&*browser_);
      break;
    case IDC_SHOW_BRAVE_VPN_PANEL:
      brave::ShowBraveVPNBubble(&*browser_);
      break;
    case IDC_TOGGLE_BRAVE_VPN_TRAY_ICON:
      brave::ToggleBraveVPNTrayIcon();
      break;
    case IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON:
      brave::ToggleBraveVPNButton(&*browser_);
      break;
    case IDC_SEND_BRAVE_VPN_FEEDBACK:
    case IDC_ABOUT_BRAVE_VPN:
    case IDC_MANAGE_BRAVE_VPN_PLAN:
      brave::OpenBraveVPNUrls(&*browser_, id);
      break;
    case IDC_SIDEBAR_TOGGLE_POSITION:
      brave::ToggleSidebarPosition(&*browser_);
      break;
    case IDC_TOGGLE_SIDEBAR:
      brave::ToggleSidebar(&*browser_);
      break;
    case IDC_COPY_CLEAN_LINK:
      brave::CopySanitizedURL(
          &*browser_,
          browser_->tab_strip_model()->GetActiveWebContents()->GetVisibleURL());
      break;
    case IDC_TOGGLE_TAB_MUTE:
      brave::ToggleActiveTabAudioMute(&*browser_);
      break;
    case IDC_TOGGLE_VERTICAL_TABS:
      brave::ToggleVerticalTabStrip(&*browser_);
      break;
    case IDC_TOGGLE_VERTICAL_TABS_WINDOW_TITLE:
      brave::ToggleWindowTitleVisibilityForVerticalTabs(&*browser_);
      break;
    case IDC_TOGGLE_VERTICAL_TABS_EXPANDED:
      brave::ToggleVerticalTabStripExpanded(&*browser_);
      break;
    case IDC_CONFIGURE_BRAVE_NEWS:
      brave::ShowBraveNewsConfigure(&*browser_);
      break;
    case IDC_CONFIGURE_SHORTCUTS:
      brave::ShowShortcutsPage(&*browser_);
      break;
    case IDC_SHOW_BRAVE_TALK:
      brave::ShowBraveTalk(&*browser_);
      break;
    case IDC_TOGGLE_SHIELDS:
      brave::ToggleShieldsEnabled(&*browser_);
      break;
    case IDC_TOGGLE_JAVASCRIPT:
      brave::ToggleJavascriptEnabled(&*browser_);
      break;
    case IDC_SHOW_PLAYLIST_BUBBLE:
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
      brave::ShowPlaylistBubble(&*browser_);
#else
      NOTREACHED_IN_MIGRATION() << " This command shouldn't be enabled";
#endif
      break;
    case IDC_SHOW_WAYBACK_MACHINE_BUBBLE:
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
      brave::ShowWaybackMachineBubble(&*browser_);
#endif
      break;
    case IDC_GROUP_TABS_ON_CURRENT_ORIGIN:
      brave::GroupTabsOnCurrentOrigin(&*browser_);
      break;
    case IDC_MOVE_GROUP_TO_NEW_WINDOW:
      brave::MoveGroupToNewWindow(&*browser_);
      break;
    case IDC_CLOSE_DUPLICATE_TABS:
      brave::CloseDuplicateTabs(&*browser_);
      break;
    case IDC_WINDOW_CLOSE_TABS_TO_LEFT:
      brave::CloseTabsToLeft(&*browser_);
      break;
    case IDC_WINDOW_CLOSE_UNPINNED_TABS:
      brave::CloseUnpinnedTabs(&*browser_);
      break;
    case IDC_WINDOW_ADD_ALL_TABS_TO_NEW_GROUP:
      brave::AddAllTabsToNewGroup(&*browser_);
      break;
    case IDC_WINDOW_MUTE_ALL_TABS:
      brave::MuteAllTabs(&*browser_, false);
      break;
    case IDC_WINDOW_MUTE_OTHER_TABS:
      brave::MuteAllTabs(&*browser_, true);
      break;
    case IDC_WINDOW_UNMUTE_ALL_TABS:
      brave::UnmuteAllTabs(&*browser_);
      break;
    case IDC_SCROLL_TAB_TO_TOP:
      brave::ScrollTabToTop(&*browser_);
      break;
    case IDC_SCROLL_TAB_TO_BOTTOM:
      brave::ScrollTabToBottom(&*browser_);
      break;
    case IDC_BRAVE_SEND_TAB_TO_SELF:
      chrome::SendTabToSelf(&*browser_);
      break;
    case IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY:
      brave::ToggleAllBookmarksButtonVisibility(base::to_address(browser_));
      break;
    case IDC_COMMANDER:
#if BUILDFLAG(ENABLE_COMMANDER)
      brave::ToggleCommander(base::to_address(browser_));
#endif
      break;
    case IDC_WINDOW_GROUP_UNGROUPED_TABS:
      brave::GroupUngroupedTabs(&*browser_);
      break;
    case IDC_WINDOW_UNGROUP_GROUP:
      brave::UngroupCurrentGroup(&*browser_);
      break;
    case IDC_WINDOW_REMOVE_TAB_FROM_GROUP:
      brave::RemoveTabFromGroup(&*browser_);
      break;
    case IDC_WINDOW_UNGROUP_ALL_TABS:
      brave::UngroupAllTabs(&*browser_);
      break;
    case IDC_WINDOW_NAME_GROUP:
      brave::NameGroup(&*browser_);
      break;
    case IDC_WINDOW_NEW_TAB_IN_GROUP:
      brave::NewTabInGroup(&*browser_);
      break;
    case IDC_WINDOW_TOGGLE_GROUP_EXPANDED:
      brave::ToggleGroupExpanded(&*browser_);
      break;
    case IDC_WINDOW_CLOSE_UNGROUPED_TABS:
      brave::CloseUngroupedTabs(&*browser_);
      break;
    case IDC_WINDOW_CLOSE_TABS_NOT_IN_CURRENT_GROUP:
      brave::CloseTabsNotInCurrentGroup(&*browser_);
      break;
    case IDC_WINDOW_CLOSE_GROUP:
      brave::CloseGroup(&*browser_);
      break;
    case IDC_WINDOW_BRING_ALL_TABS:
      brave::BringAllTabs(&*browser_);
      break;
    case IDC_NEW_SPLIT_VIEW:
      brave::NewSplitViewForTab(&*browser_);
      break;
    case IDC_TILE_TABS:
      brave::TileTabs(&*browser_);
      break;
    case IDC_BREAK_TILE:
      brave::BreakTiles(&*browser_);
      break;
    case IDC_SWAP_SPLIT_VIEW:
      brave::SwapTabsInTile(&*browser_);
      break;
    default:
      LOG(WARNING) << "Received Unimplemented Command: " << id;
      break;
  }

  return true;
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void BraveBrowserCommandController::OnPurchasedStateChanged(
    brave_vpn::mojom::PurchasedState state,
    const std::optional<std::string>& description) {
  UpdateCommandForBraveVPN();
}
#endif

}  // namespace chrome
