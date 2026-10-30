/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_

#include <optional>

#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "components/version_info/channel.h"
#include "ui/views/bubble/bubble_border.h"

class BrowserWindowInterface;
class GURL;
class PrefService;
class Profile;
class SidebarWebPanelController;
class TabStripModel;
enum class SidePanelEntryId;

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {

bool CanUseSidebar(BrowserWindowInterface* browser);
bool CanAddCurrentActiveTabToSidebar(BrowserWindowInterface* browser);
bool IsWebPanelFeatureEnabled();

// True if `focused_contents`'s focus event is web-panel-related: either its own
// contents area was clicked/focused, or focus is moving from the panel back
// to a normal tab.
bool IsWebPanelRelatedFocusChange(
    SidebarWebPanelController* web_panel_conteroller,
    TabStripModel* tab_strip_model,
    content::WebContents* focused_contents);

// Exported for testing.
bool HiddenDefaultSidebarItemsContains(SidebarService* service,
                                       const GURL& url);
GURL ConvertURLToBuiltInItemURL(const GURL& url);
SidePanelEntryId SidePanelIdFromSideBarItemType(
    SidebarItem::BuiltInItemType type);
SidePanelEntryId SidePanelIdFromSideBarItem(const SidebarItem& item);
std::optional<SidebarItem::BuiltInItemType> BuiltInItemTypeFromSidePanelId(
    SidePanelEntryId id);
void SetLastUsedSidePanel(PrefService* prefs,
                          std::optional<SidePanelEntryId> id);
std::optional<SidePanelEntryId> GetLastUsedSidePanel(
    BrowserWindowInterface* browser);

// Return the added item if item for |id| is added.
std::optional<SidebarItem> AddItemForSidePanelIdIfNeeded(Profile* profile,
                                                         SidePanelEntryId id);

bool IsDisabledItemForPrivate(SidebarItem::BuiltInItemType type);
bool IsDisabledItemForGuest(SidebarItem::BuiltInItemType type);

SidebarService::ShowSidebarOption GetDefaultShowSidebarOption(
    version_info::Channel channel);

// Get bubble arrow direction based on sidebar position.
// Returns RIGHT_TOP if sidebar is on right.
views::BubbleBorder::Arrow GetBubbleArrowForSidebar(PrefService* prefs);

// Both of these close synchronously, so the panel's tab is gone before the
// caller's next statement - the shutdown paths snapshot the session there, and
// would otherwise persist the panel as an ordinary pinned tab. Only valid once
// a close can no longer be cancelled, hence shutdown-only.

// Closes the open web panel of every browser window, across all profiles.
// Called when quitting, before session services are shut down.
void CloseAllWebPanelsForShutdown();

// Closes `browser`'s web panel. No-op when `browser` has no sidebar controller
// or no open web panel. Called when a single window is closing, before the
// session and tab restore services snapshot it.
void MaybeCloseWebPanelForShutdown(BrowserWindowInterface* browser);

// True when `contents` is `browser`'s open web panel. The panel is backed by a
// pinned tab, so pinned tab persistence has to skip it.
bool IsWebPanelTab(BrowserWindowInterface* browser,
                   const content::WebContents* contents);

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_
