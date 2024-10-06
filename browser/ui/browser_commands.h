/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
#define BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_

#include <optional>
#include <vector>

#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/tab_model.h"

class Browser;
class GURL;
class Profile;

namespace brave {

bool HasSelectedURL(Browser* browser);
void CleanAndCopySelectedURL(Browser* browser);
void NewOffTheRecordWindowTor(Browser* browser);
void NewOffTheRecordWindowTor(Profile* profile);

void NewTorConnectionForSite(Browser*);

void ToggleAIChat(Browser* browser);

void ShowWalletBubble(Browser* browser);
void ShowApproveWalletBubble(Browser* browser);
void CloseWalletBubble(Browser* browser);
void MaybeDistillAndShowSpeedreaderBubble(Browser* browser);
void ShowBraveVPNBubble(Browser* browser);
void ToggleBraveVPNButton(Browser* browser);
void ToggleBraveVPNTrayIcon();
void OpenBraveVPNUrls(Browser* browser, int command_id);
// Copies an url sanitized by URLSanitizerService.
void CopySanitizedURL(Browser* browser, const GURL& url);
// Copies an url cleared through:
// - Debouncer (potentially debouncing many levels)
// - Query filter
// - URLSanitizerService
void CopyLinkWithStrictCleaning(Browser* browser, const GURL& url);

void ToggleWindowTitleVisibilityForVerticalTabs(Browser* browser);
void ToggleVerticalTabStrip(Browser* browser);
void ToggleVerticalTabStripFloatingMode(Browser* browser);
void ToggleVerticalTabStripExpanded(Browser* browser);

void ToggleActiveTabAudioMute(Browser* browser);
void ToggleSidebarPosition(Browser* browser);
void ToggleSidebar(Browser* browser);

void ToggleShieldsEnabled(Browser* browser);
void ToggleJavascriptEnabled(Browser* browser);

#if BUILDFLAG(ENABLE_COMMANDER)
void ToggleCommander(Browser* browser);
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
void ShowPlaylistBubble(Browser* browser);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
void ShowWaybackMachineBubble(Browser* browser);
#endif

void GroupTabsOnCurrentOrigin(Browser* browser);
void MoveGroupToNewWindow(Browser* browser);

bool IsInGroup(Browser* browser);
bool HasUngroupedTabs(Browser* browser);

void GroupUngroupedTabs(Browser* browser);
void UngroupCurrentGroup(Browser* browser);
void RemoveTabFromGroup(Browser* browser);
void NameGroup(Browser* browser);
void NewTabInGroup(Browser* browser);

bool CanUngroupAllTabs(Browser* browser);
void UngroupAllTabs(Browser* browser);

void ToggleGroupExpanded(Browser* browser);
void CloseUngroupedTabs(Browser* browser);
void CloseTabsNotInCurrentGroup(Browser* browser);
void CloseGroup(Browser* browser);

bool CanBringAllTabs(Browser* browser);
void BringAllTabs(Browser* browser);

bool HasDuplicateTabs(Browser* browser);
void CloseDuplicateTabs(Browser* browser);

bool CanCloseTabsToLeft(Browser* browser);
void CloseTabsToLeft(Browser* browser);

bool CanCloseUnpinnedTabs(Browser* browser);
void CloseUnpinnedTabs(Browser* browser);

void AddAllTabsToNewGroup(Browser* browser);

bool CanMuteAllTabs(Browser* browser, bool exclude_active);
void MuteAllTabs(Browser* browser, bool exclude_active);

bool CanUnmuteAllTabs(Browser* browser);
void UnmuteAllTabs(Browser* browser);

void ScrollTabToTop(Browser* browser);
void ScrollTabToBottom(Browser* browser);

void ExportAllBookmarks(Browser* browser);
void ToggleAllBookmarksButtonVisibility(Browser* browser);

// In case |tab| is not provided, the active tab will be used.
bool CanOpenNewSplitViewForTab(
    Browser* browser,
    std::optional<tabs::TabHandle> tab = std::nullopt);
void NewSplitViewForTab(Browser* browser,
                        std::optional<tabs::TabHandle> tab = std::nullopt,
                        const GURL& url = GURL());
// In case |indices| empty, selected tabs will be used.
void TileTabs(Browser* browser, const std::vector<int>& indices = {});
void BreakTiles(Browser* browser, const std::vector<int>& indices = {});
bool IsTabsTiled(Browser* browser, const std::vector<int>& indices = {});
bool CanTileTabs(Browser* browser, const std::vector<int>& indices = {});
void SwapTabsInTile(Browser* browser);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
