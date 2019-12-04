/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_

#include "brave/browser/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "chrome/browser/ui/views/infobars/infobar_view.h"

namespace content {
class WebContents;
}  // namespace content

class BraveWaybackMachineInfoBarDelegate;

class BraveWaybackMachineInfoBarView : public InfoBarView,
                                       public WaybackMachineURLFetcher::Client {
 public:
  BraveWaybackMachineInfoBarView(
      std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
      content::WebContents* contents);
  ~BraveWaybackMachineInfoBarView() override;

  BraveWaybackMachineInfoBarView(
      const BraveWaybackMachineInfoBarView&) = delete;
  BraveWaybackMachineInfoBarView& operator=(
      const BraveWaybackMachineInfoBarView&) = delete;

 private:
  class InfoBarViewSubViews;

  // InfoBarView overrides:
  void Layout() override;

  // WaybackMachineURLFetcher::Client overrides:
  void OnWaybackURLFetched(const GURL& latest_wayback_url) override;

  void FetchWaybackURL();
  void LoadURL(const GURL& url);

  InfoBarViewSubViews* sub_views_ = nullptr;
  content::WebContents* contents_;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_
