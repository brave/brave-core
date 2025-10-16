/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include <vector>

#include "base/check.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace {

// If |selected_tabs| only has two tabs from same split tab,
// make |selected_tabs| includes only active split tab to reload
// active tab not the both tab.
void MakeActiveTabReloadOnlyForSplitTab(
    TabStripModel* tab_strip_model,
    std::vector<content::WebContents*>& selected_tabs) {
  if (selected_tabs.size() != 2) {
    return;
  }

  // Only have interests when two tabs are in the same split tab.
  auto* first_tab = tabs::TabInterface::GetFromContents(selected_tabs[0]);
  if (!first_tab->GetSplit()) {
    return;
  }

  // Only give active tab if two tabs are in the same split tab.
  auto* second_tab = tabs::TabInterface::GetFromContents(selected_tabs[1]);
  if (first_tab->GetSplit() == second_tab->GetSplit()) {
    auto* active_web_contents = tab_strip_model->GetActiveWebContents();
    if (selected_tabs[0] != active_web_contents &&
        selected_tabs[1] != active_web_contents) {
      return;
    }
    auto* reload_target =
        first_tab->IsActivated() ? selected_tabs[0] : selected_tabs[1];
    selected_tabs.clear();
    selected_tabs.push_back(reload_target);
  }
}

}  // namespace

#define ReloadBypassingCache ReloadBypassingCache_ChromiumImpl
#define GetReadingListModel GetReadingListModel_ChromiumImpl
#define kChromeUISplitViewNewTabPageURL kChromeUINewTabURL

// Need to patch to adjust |selected_tabs| in the middle of ReloadInternal().
#define BRAVE_RELOAD_INTERNAL \
  MakeActiveTabReloadOnlyForSplitTab(tab_strip_model, tabs_to_reload);

#include <chrome/browser/ui/browser_commands.cc>

#undef BRAVE_RELOAD_INTERNAL
#undef kChromeUISplitViewNewTabPageURL
#undef ReloadBypassingCache
#undef GetReadingListModel

namespace chrome {

void ReloadBypassingCache(Browser* browser, WindowOpenDisposition disposition) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->profile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (profile->IsTor()) {
    brave::NewTorConnectionForSite(browser);
    return;
  }
#endif
  ReloadBypassingCache_ChromiumImpl(browser, disposition);
}

ReadingListModel* GetReadingListModel(Browser* browser) {
  return nullptr;
}

}  // namespace chrome
