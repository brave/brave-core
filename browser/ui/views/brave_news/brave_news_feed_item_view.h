// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_FEED_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_FEED_ITEM_VIEW_H_

#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/leo/leo_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}

class BraveNewsFeedItemView : public views::View,
                              public BraveNewsTabHelper::PageFeedsObserver {
 public:
  METADATA_HEADER(BraveNewsFeedItemView);

  explicit BraveNewsFeedItemView(BraveNewsTabHelper::FeedDetails details,
                                 content::WebContents* contents);
  BraveNewsFeedItemView(const BraveNewsFeedItemView&) = delete;
  BraveNewsFeedItemView& operator=(const BraveNewsFeedItemView&) = delete;
  ~BraveNewsFeedItemView() override;

  void Update();

  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override;

  void OnPressed();

 private:
  bool loading_ = false;
  raw_ptr<leo::LeoButton> subscribe_button_ = nullptr;

  BraveNewsTabHelper::FeedDetails feed_details_;
  raw_ptr<content::WebContents> contents_;
  raw_ptr<BraveNewsTabHelper> tab_helper_;

  base::ScopedObservation<BraveNewsTabHelper,
                          BraveNewsTabHelper::PageFeedsObserver>
      tab_helper_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_FEED_ITEM_VIEW_H_
