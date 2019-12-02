/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/views/infobars/infobar_view.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace content {
class WebContents;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // network

class BraveWaybackMachineInfoBarDelegate;

class BraveWaybackMachineInfoBarView : public InfoBarView {
 public:
  BraveWaybackMachineInfoBarView(
      std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
      content::WebContents* contents);
  ~BraveWaybackMachineInfoBarView() override;

 private:
  class InfoBarViewSubViews;

  // InfoBarView overrides:
  void Layout() override;

  void OnWaybackURLFetched(std::unique_ptr<std::string> response_body);
  void LoadURL(const std::string& last_wayback_url);
  void FetchWaybackURL();

  InfoBarViewSubViews* sub_views_ = nullptr;
  content::WebContents* contents_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> wayback_url_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(BraveWaybackMachineInfoBarView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_
