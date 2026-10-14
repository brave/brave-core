/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
#define BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_

#include <optional>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "ui/events/event_constants.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace actions {
class ActionItem;
}

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#endif

class BrowserWindowInterface;
class Profile;

namespace brave {

bool HasSelectedURL(BrowserWindowInterface* browser);
void CleanAndCopySelectedURL(BrowserWindowInterface* browser);

#if BUILDFLAG(ENABLE_TOR)
void NewOffTheRecordWindowTor(BrowserWindowInterface* browser);
void NewOffTheRecordWindowTor(Profile* profile);
void NewTorConnectionForSite(BrowserWindowInterface*);
#endif

// Toggles the given side panel entry: closes the sidebar if the entry is
// already showing, switches to the entry if the sidebar is open on a different
// entry, or opens the sidebar on the entry if it's closed.
void ToggleSidePanel(BrowserWindowInterface* browser, SidePanelEntryId id);

void ToggleAIChat(BrowserWindowInterface* browser);

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
void ShowWalletBubble(BrowserWindowInterface* browser);
void CloseWalletBubble(BrowserWindowInterface* browser);
#endif

void MaybeDistillAndShowSpeedreaderBubble(BrowserWindowInterface* browser);
void ShowBraveVPNBubble(BrowserWindowInterface* browser);
void ToggleBraveVPNButton(BrowserWindowInterface* browser);
void ToggleBraveVPNTrayIcon();
void OpenBraveVPNUrls(BrowserWindowInterface* browser, int command_id);
// Copies an url sanitized by URLSanitizerService.
void CopySanitizedURL(BrowserWindowInterface* browser, const GURL& url);
// Copies an url cleared through:
// - Debouncer (potentially debouncing many levels)
// - Query filter
// - URLSanitizerService
void CopyLinkWithStrictCleaning(BrowserWindowInterface* browser,
                                const GURL& url);

void ToggleWindowTitleVisibilityForVerticalTabs(
    BrowserWindowInterface* browser);
void ToggleVerticalTabStrip(BrowserWindowInterface* browser);
void ToggleVerticalTabStripFloatingMode(BrowserWindowInterface* browser);
void ToggleVerticalTabStripExpanded(BrowserWindowInterface* browser);

void ToggleActiveTabAudioMute(BrowserWindowInterface* browser);
void ToggleSidebarPosition(BrowserWindowInterface* browser);
void ToggleSidebar(BrowserWindowInterface* browser);

void ToggleFocusMode(BrowserWindowInterface* browser);

void ToggleShieldsEnabled(BrowserWindowInterface* browser);
void ToggleJavascriptEnabled(BrowserWindowInterface* browser);

// Launches the element picker ("Block elements") for the browser's active tab,
// if the page and Shields settings support it. Backs IDC_BLOCK_ELEMENTS so
// users can assign a custom keyboard shortcut. Implemented in the cosmetic
// filters layer (which can't be depended on from here) to reach the picker.
void LaunchContentPicker(BrowserWindowInterface* browser);

#if BUILDFLAG(ENABLE_COMMANDER)
void ToggleCommander(BrowserWindowInterface* browser);
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
void ShowPlaylistBubble(BrowserWindowInterface* browser);
#endif

void GroupTabsOnCurrentOrigin(BrowserWindowInterface* browser);
void MoveGroupToNewWindow(BrowserWindowInterface* browser);

bool IsInGroup(BrowserWindowInterface* browser);
bool HasUngroupedTabs(BrowserWindowInterface* browser);

void GroupUngroupedTabs(BrowserWindowInterface* browser);
void UngroupCurrentGroup(BrowserWindowInterface* browser);
void RemoveTabFromGroup(BrowserWindowInterface* browser);
void NameGroup(BrowserWindowInterface* browser);
void NewTabInGroup(BrowserWindowInterface* browser);

bool CanUngroupAllTabs(BrowserWindowInterface* browser);
void UngroupAllTabs(BrowserWindowInterface* browser);

void ToggleGroupExpanded(BrowserWindowInterface* browser);
void CloseUngroupedTabs(BrowserWindowInterface* browser);
void CloseTabsNotInCurrentGroup(BrowserWindowInterface* browser);
void CloseGroup(BrowserWindowInterface* browser);

bool CanBringAllTabs(BrowserWindowInterface* browser);
void BringAllTabs(BrowserWindowInterface* browser);

bool HasDuplicatesOfActiveTab(BrowserWindowInterface* browser);
void CloseDuplicatesOfActiveTab(BrowserWindowInterface* browser);
bool HasAnyDuplicateTabs(BrowserWindowInterface* browser);
void CloseAllDuplicateTabs(BrowserWindowInterface* browser);
bool CanCloseTabsToLeft(BrowserWindowInterface* browser);
void CloseTabsToLeft(BrowserWindowInterface* browser);

bool CanCloseUnpinnedTabs(BrowserWindowInterface* browser);
void CloseUnpinnedTabs(BrowserWindowInterface* browser);

void AddAllTabsToNewGroup(BrowserWindowInterface* browser);

bool CanMuteAllTabs(BrowserWindowInterface* browser, bool exclude_active);
void MuteAllTabs(BrowserWindowInterface* browser, bool exclude_active);

bool CanUnmuteAllTabs(BrowserWindowInterface* browser);
void UnmuteAllTabs(BrowserWindowInterface* browser);

void ScrollTabToTop(BrowserWindowInterface* browser);
void ScrollTabToBottom(BrowserWindowInterface* browser);

void ExportAllBookmarks(BrowserWindowInterface* browser);
void ToggleAllBookmarksButtonVisibility(BrowserWindowInterface* browser);

// Split view API with SideBySide.
// false if active tab is already split tab.
bool CanOpenNewSplitTabsWithSideBySide(BrowserWindowInterface* browser);

// true if two tabs are selected and both are not in split tabs.
bool CanSplitTabsWithSideBySide(BrowserWindowInterface* browser);

// Add to split with selected two tabs.
void SplitTabsWithSideBySide(BrowserWindowInterface* browser,
                             split_tabs::SplitTabCreatedSource source);

// true if any selected tab is split tabs.
bool IsSplitTabs(BrowserWindowInterface* browser);

// Remove split tabs of selected tabs.
void RemoveSplitWithSideBySide(BrowserWindowInterface* browser);

// Swap tabs in active tab.
void SwapTabsInSplitWithSideBySide(BrowserWindowInterface* browser);

// Force pastes into the active web contents in the browser, if focused.
void ForcePasteInBrowser(BrowserWindowInterface* browser);

// Force pastes into the web contents if focused.
void ForcePasteInWebContents(content::WebContents* contents);

#if BUILDFLAG(ENABLE_CONTAINERS)
// Creates new tabs with the given tabs' URLs in the specified container.
void OpenTabUrlsInContainer(BrowserWindowInterface* bwi,
                            const std::vector<tabs::TabHandle>& tabs,
                            const containers::mojom::ContainerPtr& container);
// Creates a new tab with the specified URL in the given container.
void OpenUrlInContainer(
    BrowserWindowInterface* bwi,
    const GURL& url,
    const containers::mojom::ContainerPtr& container,
    bool is_link = true,
    std::optional<url::Origin> initiator_origin = std::nullopt,
    bool started_from_context_menu = false);

// Creates new tabs with the given tabs' URLs without a container.
void OpenTabUrlsWithoutContainer(BrowserWindowInterface* bwi,
                                 const std::vector<tabs::TabHandle>& tabs);
void OpenUrlWithoutContainer(
    BrowserWindowInterface* bwi,
    const GURL& url,
    bool is_link = true,
    std::optional<url::Origin> initiator_origin = std::nullopt,
    bool started_from_context_menu = false);

// Creates a new temporary container and opens the given tabs' URLs in it.
void CreateTemporaryContainerAndOpenTabUrls(
    BrowserWindowInterface* bwi,
    const std::vector<tabs::TabHandle>& tabs);
// Opens |url| in a new tab in a freshly created temporary container.
void CreateTemporaryContainerAndOpenUrl(
    BrowserWindowInterface* bwi,
    const GURL& url,
    bool is_link = true,
    std::optional<url::Origin> initiator_origin = std::nullopt,
    bool started_from_context_menu = false);

// Opens the container menu on the page action view if the active tab is in a
// container.
void OpenContainerMenuOnPageActionView(BrowserWindowInterface* bwi,
                                       ::actions::ActionItem* item);
#endif

#if BUILDFLAG(ENABLE_PSST)
void OpenPsstMenuOnPageActionView(BrowserWindowInterface* bwi,
                                  actions::ActionItem* item,
                                  int event_flags = ui::EF_NONE);
#endif

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
