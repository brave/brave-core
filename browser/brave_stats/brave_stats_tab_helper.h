/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace brave_stats {

class BraveStatsUpdater;

class BraveStatsTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveStatsTabHelper> {
 public:
  explicit BraveStatsTabHelper(content::WebContents*);
  ~BraveStatsTabHelper() override;
  BraveStatsTabHelper(const BraveStatsTabHelper&) = delete;
  BraveStatsTabHelper& operator=(const BraveStatsTabHelper&) = delete;

  void NotifyStatsUpdater();

 private:
  void DidStartNavigation(content::NavigationHandle*) override;

  friend class content::WebContentsUserData<BraveStatsTabHelper>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_stats
#endif  // BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_TAB_HELPER_H_
