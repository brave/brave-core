/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_utils.h"

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/constants.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"

namespace sidebar {

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

}  // namespace sidebar
