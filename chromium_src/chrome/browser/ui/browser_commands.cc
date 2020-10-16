/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/ui/browser_commands.h"

#define ReloadBypassingCache ReloadBypassingCache_ChromiumImpl
#define NewTab NewTab_ChromiumImpl_Unused
#include "../../../../../chrome/browser/ui/browser_commands.cc"  // NOLINT
#undef ReloadBypassingCache
#undef NewTab

namespace chrome {

// Whole logic is copied from upstream chrome::NewTab().
void NewTab(Browser* browser) {
  base::RecordAction(UserMetricsAction("NewTab"));
  // TODO(asvitkine): This is invoked programmatically from several places.
  // Audit the code and change it so that the histogram only gets collected for
  // user-initiated commands.
  UMA_HISTOGRAM_ENUMERATION("Tab.NewTab", TabStripModel::NEW_TAB_COMMAND,
                            TabStripModel::NEW_TAB_ENUM_COUNT);

  GURL new_tab_url = brave::GetNewTabPageURL(browser->profile());

  // Notify IPH that new tab was opened.
  auto* reopen_tab_iph =
      ReopenTabInProductHelpFactory::GetForProfile(browser->profile());
  reopen_tab_iph->NewTabOpened();

  if (browser->SupportsWindowFeature(Browser::FEATURE_TABSTRIP)) {
    AddTabAt(browser, new_tab_url, -1, true);
  } else {
    ScopedTabbedBrowserDisplayer displayer(browser->profile());
    Browser* b = displayer.browser();
    AddTabAt(b, new_tab_url, -1, true);
    b->window()->Show();
    // The call to AddBlankTabAt above did not set the focus to the tab as its
    // window was not active, so we have to do it explicitly.
    // See http://crbug.com/6380.
    b->tab_strip_model()->GetActiveWebContents()->RestoreFocus();
  }
}

void ReloadBypassingCache(Browser* browser, WindowOpenDisposition disposition) {
  Profile* profile = browser->profile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (brave::IsTorProfile(profile))
    brave::NewTorConnectionForSite(browser);
  else
    ReloadBypassingCache_ChromiumImpl(browser, disposition);
}

}  // namespace chrome
