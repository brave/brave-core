// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_TAB_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class RenderFrameHost;
}

class BraveNewsTabHelper
    : public content::WebContentsUserData<BraveNewsTabHelper>,
      public content::WebContentsObserver,
      public brave_news::BraveNewsPrefManager::PrefObserver {
 public:
  struct FeedDetails {
    GURL feed_url;
    std::string title = "";
    bool subscribed = false;

    // The combined publisher id, if any. Empty string if not a combined
    // publisher.
    std::string combined_publisher_id = "";

    // Indicates whether we've requested this feed, so we don't request it
    // multiple times.
    bool requested_feed = false;

    FeedDetails();
    FeedDetails(const FeedDetails&) = delete;
    FeedDetails& operator=(const FeedDetails&) = delete;
    FeedDetails(FeedDetails&&);
    FeedDetails& operator=(FeedDetails&&);
    ~FeedDetails();
  };

  class PageFeedsObserver : public base::CheckedObserver {
   public:
    virtual void OnAvailableFeedsChanged(
        const std::vector<GURL>& feed_urls) = 0;
  };

  static void MaybeCreateForWebContents(content::WebContents* contents);

  BraveNewsTabHelper(const BraveNewsTabHelper&) = delete;
  BraveNewsTabHelper& operator=(const BraveNewsTabHelper&) = delete;

  ~BraveNewsTabHelper() override;

  const std::vector<GURL> GetAvailableFeedUrls();
  bool IsSubscribed(const GURL& feed_url);
  bool IsSubscribed();

  void ToggleSubscription(const GURL& feed_details);
  std::string GetTitleForFeedUrl(const GURL& url);

  void OnReceivedRssUrls(const GURL& site_url, std::vector<GURL> feed_urls);
  void OnFoundFeedData(const GURL& feed_url,
                       const GURL& site_url,
                       std::vector<brave_news::mojom::FeedSearchResultItemPtr>);

  void AddObserver(PageFeedsObserver* observer);
  void RemoveObserver(PageFeedsObserver* observer);

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void DOMContentLoaded(content::RenderFrameHost* rfh) override;

  // brave_news::BraveNewsPrefManager::PrefObserver:
  void OnPublishersChanged() override;

 private:
  explicit BraveNewsTabHelper(content::WebContents* contents);

  bool ShouldFindFeeds();
  void OnReceivedNewPublishers(brave_news::Publishers publishers);
  void AvailableFeedsChanged();
  void UpdatePageFeed();

  raw_ptr<brave_news::BraveNewsController> controller_;

  std::vector<FeedDetails> rss_page_feeds_;

  // The (optional) publisher associated with this page from our
  // PublishersController. This may be duplicated by one of the
  // |rss_page_feeds_| so we should ensure we deduplicate based on |feed_url|.
  brave_news::mojom::PublisherPtr default_feed_ = nullptr;

  base::ObserverList<PageFeedsObserver> observers_;

  base::ScopedObservation<brave_news::BraveNewsPrefManager,
                          brave_news::BraveNewsPrefManager::PrefObserver>
      pref_observation_{this};

  base::WeakPtrFactory<BraveNewsTabHelper> weak_ptr_factory_{this};

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_TAB_HELPER_H_
