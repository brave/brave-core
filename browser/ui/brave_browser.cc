/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "chrome/browser/search/search.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#endif

BraveBrowser::BraveBrowser(const CreateParams& params) : Browser(params) {
#if BUILDFLAG(ENABLE_SIDEBAR)
  if (!sidebar::CanUseSidebar(profile()) || !is_type_normal())
    return;
  // Below call order is important.
  // When reaches here, Sidebar UI is setup in BraveBrowserView but
  // not initialized. It's just empty because sidebar controller/model is not
  // ready yet. BraveBrowserView is instantiated by the ctor of Browser.
  // So, initializing sidebar controller/model here and then ask to initialize
  // sidebar UI. After that, UI will be updated for model's change.
  sidebar_controller_.reset(new sidebar::SidebarController(this, profile()));
  sidebar_controller_->SetSidebar(brave_window()->InitSidebar());
#endif
}

BraveBrowser::~BraveBrowser() = default;

void BraveBrowser::ScheduleUIUpdate(content::WebContents* source,
                                    unsigned changed_flags) {
  Browser::ScheduleUIUpdate(source, changed_flags);

#if BUILDFLAG(ENABLE_SIDEBAR)
  if (tab_strip_model_->GetIndexOfWebContents(source) == TabStripModel::kNoTab)
    return;

  // We need to update sidebar UI only when current active tab state is changed.
  if (changed_flags & content::INVALIDATE_TYPE_URL) {
    if (source == tab_strip_model_->GetActiveWebContents()) {
      if (sidebar_controller_)
        sidebar_controller_->sidebar()->UpdateSidebar();
    }
  }
#endif
}

bool BraveBrowser::ShouldDisplayFavicon(
    content::WebContents* web_contents) const {
  // Override to not show favicon for NTP in tab.

  // Suppress the icon for the new-tab page, even if a navigation to it is
  // not committed yet. Note that we're looking at the visible URL, so
  // navigations from NTP generally don't hit this case and still show an icon.
  GURL url = web_contents->GetVisibleURL();
  if (url.SchemeIs(content::kChromeUIScheme) &&
      url.host_piece() == chrome::kChromeUINewTabHost) {
    return false;
  }

  // Also suppress instant-NTP. This does not use search::IsInstantNTP since
  // it looks at the last-committed entry and we need to show icons for pending
  // navigations away from it.
  if (search::IsInstantNTPURL(url, profile())) {
    return false;
  }

  return Browser::ShouldDisplayFavicon(web_contents);
}

void BraveBrowser::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  Browser::OnTabStripModelChanged(tab_strip_model, change, selection);

#if BUILDFLAG(ENABLE_SIDEBAR)
  // We need to update sidebar UI whenever active tab is changed.
  if (selection.active_tab_changed() && sidebar_controller_)
    sidebar_controller_->sidebar()->UpdateSidebar();
#endif
}

BraveBrowserWindow* BraveBrowser::brave_window() {
  return static_cast<BraveBrowserWindow*>(window_);
}
