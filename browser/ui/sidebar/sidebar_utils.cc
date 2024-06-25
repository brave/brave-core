/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_utils.h"

#include <optional>

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"

namespace sidebar {

using BuiltInItemType = SidebarItem::BuiltInItemType;
using ShowSidebarOption = SidebarService::ShowSidebarOption;

namespace {

SidebarService* GetSidebarService(Browser* browser) {
  return SidebarServiceFactory::GetForProfile(browser->profile());
}

bool IsActiveTabNTP(content::WebContents* active_web_contents) {
  content::NavigationEntry* entry =
      active_web_contents->GetController().GetLastCommittedEntry();
  if (!entry)
    entry = active_web_contents->GetController().GetVisibleEntry();
  if (!entry)
    return false;
  const GURL url = entry->GetURL();
  return NewTabUI::IsNewTab(url) || NewTabPageUI::IsNewTabPageOrigin(url) ||
         search::NavEntryIsInstantNTP(active_web_contents, entry);
}

bool IsURLAlreadyAddedToSidebar(SidebarService* service, const GURL& url) {
  const GURL converted_url = ConvertURLToBuiltInItemURL(url);
  for (const auto& item : service->items()) {
    if (item.url == converted_url)
      return true;
  }

  return false;
}

}  // namespace

bool HiddenDefaultSidebarItemsContains(SidebarService* service,
                                       const GURL& url) {
  const auto not_added_default_items = service->GetHiddenDefaultSidebarItems();
  if (not_added_default_items.empty())
    return false;
  const GURL converted_url = ConvertURLToBuiltInItemURL(url);
  for (const auto& item : not_added_default_items) {
    if (item.url == converted_url)
      return true;
  }
  return false;
}

bool CanUseSidebar(Browser* browser) {
  DCHECK(browser);
  return browser->is_type_normal();
}

// If url is relavant with bulitin items, use builtin item's url.
// Ex, we don't need to add bookmarks manager as a sidebar shortcut
// if sidebar panel already has bookmarks item.
GURL ConvertURLToBuiltInItemURL(const GURL& url) {
  if (url == GURL(chrome::kChromeUIBookmarksURL))
    return GURL(chrome::kChromeUIBookmarksSidePanelURL);

  if (url.host() == kBraveTalkHost)
    return GURL(kBraveTalkURL);

  if (url.SchemeIs(content::kChromeUIScheme) && url.host() == kWalletPageHost) {
    return GURL(kBraveUIWalletPageURL);
  }
  return url;
}

bool CanAddCurrentActiveTabToSidebar(Browser* browser) {
  auto* active_web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!active_web_contents)
    return false;

  if (IsActiveTabNTP(active_web_contents))
    return false;

  const GURL url = active_web_contents->GetLastCommittedURL();
  if (!url.is_valid())
    return false;

  auto* service = GetSidebarService(browser);
  if (IsURLAlreadyAddedToSidebar(service, url))
    return false;

  if (HiddenDefaultSidebarItemsContains(service, url))
    return false;

  return true;
}

SidePanelEntryId SidePanelIdFromSideBarItemType(BuiltInItemType type) {
  switch (type) {
    case BuiltInItemType::kReadingList:
      return SidePanelEntryId::kReadingList;
    case BuiltInItemType::kBookmarks:
      return SidePanelEntryId::kBookmarks;
    case BuiltInItemType::kPlaylist:
      return SidePanelEntryId::kPlaylist;
    case BuiltInItemType::kChatUI:
      return SidePanelEntryId::kChatUI;
    case BuiltInItemType::kWallet:
      [[fallthrough]];
    case BuiltInItemType::kBraveTalk:
      [[fallthrough]];
    case BuiltInItemType::kHistory:
      [[fallthrough]];
    case BuiltInItemType::kNone:
      // Add a new case for any new types which we want to support.
      NOTREACHED_IN_MIGRATION()
          << "Asked for a panel Id from a sidebar item which should "
             "not have a panel Id, sending Reading List instead.";
      return SidePanelEntryId::kReadingList;
  }
}

SidePanelEntryId SidePanelIdFromSideBarItem(const SidebarItem& item) {
  DCHECK(item.open_in_panel);
  return SidePanelIdFromSideBarItemType(item.built_in_item_type);
}

std::optional<SidebarItem> AddItemForSidePanelIdIfNeeded(Browser* browser,
                                                         SidePanelEntryId id) {
  const auto hidden_default_items =
      GetSidebarService(browser)->GetHiddenDefaultSidebarItems();
  if (hidden_default_items.empty()) {
    return std::nullopt;
  }

  for (const auto& item : hidden_default_items) {
    if (id == sidebar::SidePanelIdFromSideBarItem(item)) {
      GetSidebarService(browser)->AddItem(item);
      return item;
    }
  }

  return std::nullopt;
}

void SetLastUsedSidePanel(PrefService* prefs,
                          std::optional<SidePanelEntryId> id) {
  BuiltInItemType type = BuiltInItemType::kNone;
  if (id) {
    switch (*id) {
      case SidePanelEntryId::kReadingList:
        type = BuiltInItemType::kReadingList;
        break;
      case SidePanelEntryId::kBookmarks:
        type = BuiltInItemType::kBookmarks;
        break;
      case SidePanelEntryId::kPlaylist:
        type = BuiltInItemType::kPlaylist;
        break;
      case SidePanelEntryId::kChatUI:
        type = BuiltInItemType::kChatUI;
        break;
      default:
        break;
    }
  }

  prefs->SetInteger(kLastUsedBuiltInItemType, static_cast<int>(type));
}

std::optional<SidePanelEntryId> GetLastUsedSidePanel(Browser* browser) {
  PrefService* prefs = browser->profile()->GetPrefs();
  BuiltInItemType type =
      static_cast<BuiltInItemType>(prefs->GetInteger(kLastUsedBuiltInItemType));
  // If cached type item is not included in current model, return null.
  if (!static_cast<BraveBrowser*>(browser)
           ->sidebar_controller()
           ->model()
           ->GetIndexOf(type)) {
    return std::nullopt;
  }
  return SidePanelIdFromSideBarItemType(type);
}

bool IsDisabledItemForPrivate(SidebarItem::BuiltInItemType type) {
  switch (type) {
    case SidebarItem::BuiltInItemType::kChatUI:
    case SidebarItem::BuiltInItemType::kPlaylist:
      return true;
    default:
      break;
  }
  return false;
}

bool IsDisabledItemForGuest(SidebarItem::BuiltInItemType type) {
  switch (type) {
    case SidebarItem::BuiltInItemType::kBookmarks:
    case SidebarItem::BuiltInItemType::kReadingList:
    case SidebarItem::BuiltInItemType::kChatUI:
    case SidebarItem::BuiltInItemType::kPlaylist:
      return true;
    default:
      break;
  }
  return false;
}

SidebarService::ShowSidebarOption GetDefaultShowSidebarOption(
    version_info::Channel channel) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDontShowSidebarOnNonStable) &&
      channel != version_info::Channel::STABLE) {
    return ShowSidebarOption::kShowAlways;
  }

  if (!g_browser_process) {
    CHECK_IS_TEST();
    return ShowSidebarOption::kShowAlways;
  }

  if (auto* local_state = g_browser_process->local_state()) {
    return local_state->GetBoolean(kTargetUserForSidebarEnabledTest)
               ? ShowSidebarOption::kShowAlways
               : ShowSidebarOption::kShowNever;
  }

  return ShowSidebarOption::kShowNever;
}

}  // namespace sidebar
