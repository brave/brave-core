/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats/brave_stats_tab_helper.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/page_transition_types.h"

namespace brave_stats {

BraveStatsTabHelper::BraveStatsTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveStatsTabHelper>(*web_contents) {}

BraveStatsTabHelper::~BraveStatsTabHelper() = default;

void BraveStatsTabHelper::DidStartNavigation(
        content::NavigationHandle* handle) {
  if (!handle || !handle->IsInMainFrame() || handle->IsDownload())
    return;

  auto transition = handle->GetPageTransition();

  switch (ui::PageTransitionStripQualifier(transition)) {
    case ui::PAGE_TRANSITION_TYPED:
    case ui::PAGE_TRANSITION_AUTO_BOOKMARK:
    case ui::PAGE_TRANSITION_GENERATED: {
      auto url = handle->GetURL();
      if (!(url.SchemeIs("chrome") || url.SchemeIs("brave")))
        NotifyStatsUpdater();
      break;
    }
    default:
      break;
  }
}

void BraveStatsTabHelper::NotifyStatsUpdater() {
  if (g_brave_browser_process->brave_stats_updater()->MaybeDoThresholdPing(1))
    GetWebContents().RemoveUserData(UserDataKey());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveStatsTabHelper);

}  //  namespace brave_stats
