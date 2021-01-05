/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_utils.h"

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"

namespace sidebar {

namespace {

SidebarService* GetSidebarService(Browser* browser) {
  return SidebarServiceFactory::GetForProfile(browser->profile());
}

bool IsActiveTabNTP(Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  if (!entry)
    entry = web_contents->GetController().GetVisibleEntry();
  if (!entry)
    return false;
  const GURL url = entry->GetURL();
  return NewTabUI::IsNewTab(url) || NewTabPageUI::IsNewTabPageOrigin(url) ||
         search::NavEntryIsInstantNTP(web_contents, entry);
}

bool IsActiveTabAlreadyAddedToSidebar(Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  const GURL url = web_contents->GetVisibleURL();
  for (const auto& item : GetSidebarService(browser)->items()) {
    if (item.url == url)
      return true;
  }

  return false;
}

}  // namespace

bool CanAddCurrentActiveTabToSidebar(Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!web_contents)
    return false;

  if (IsActiveTabNTP(browser))
    return false;

  if (IsActiveTabAlreadyAddedToSidebar(browser))
    return false;

  return true;
}

}  // namespace sidebar
