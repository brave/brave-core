// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_TAB_HELPER_H_

#include <vector>
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/views/widget/widget.h"

class Page;

class BraveNewsTabHelper
    : public content::WebContentsUserData<BraveNewsTabHelper>,
      public content::WebContentsObserver,
      public brave_news::PublishersController::Observer {
 public:
  struct FeedDetails {
   public:
    GURL feed_url;
    std::string publisher_id;
    std::string title;
  };

  class PageFeedsObserver {
   public:
    virtual void OnAvailableFeedsChanged(
        const std::vector<FeedDetails>& feeds) = 0;
  };

  BraveNewsTabHelper(const BraveNewsTabHelper&) = delete;
  BraveNewsTabHelper& operator=(const BraveNewsTabHelper&) = delete;

  ~BraveNewsTabHelper() override;

  const std::vector<FeedDetails>& available_feeds() const { return available_feeds_; }
  bool is_subscribed(const FeedDetails& feed_details);
  bool is_subscribed();

  void ToggleSubscription(const FeedDetails& feed_details);

  void OnReceivedRssUrls(const GURL& site_url, std::vector<GURL> feed_urls);
  void OnFoundFeeds(const GURL& site_url,
                    std::vector<brave_news::mojom::FeedSearchResultItemPtr>);

  void AddObserver(PageFeedsObserver* observer);
  void RemoveObserver(PageFeedsObserver* observer);

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

  // brave_news::PublisherController::Observer:
  void OnPublishersUpdated(
      brave_news::PublishersController* controller) override;

 private:
  explicit BraveNewsTabHelper(content::WebContents* contents);

  void AvailableFeedsChanged();

  raw_ptr<brave_news::BraveNewsController> controller_;

  std::vector<FeedDetails> available_feeds_;
  std::vector<PageFeedsObserver*> observers_;

  base::WeakPtrFactory<BraveNewsTabHelper> weak_ptr_factory_{this};

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_TAB_HELPER_H_
