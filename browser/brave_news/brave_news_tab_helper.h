// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_TAB_HELPER_H_

#include <memory>
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
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_NEWS));

namespace content {
class RenderFrameHost;
}

namespace tabs {
class TabInterface;
}  // namespace tabs

// Finds the Brave News feeds of the page shown in a tab. Owned by
// BraveTabFeatures.
class BraveNewsTabHelper
    : public tabs::ContentsObservingTabFeature,
      public brave_news::BraveNewsPrefManager::PrefObserver {
 public:
  DECLARE_USER_DATA(BraveNewsTabHelper);

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

  static BraveNewsTabHelper* From(tabs::TabInterface* tab);

  // Returns nullptr off the record.
  static std::unique_ptr<BraveNewsTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  explicit BraveNewsTabHelper(tabs::TabInterface& tab);
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

  void SetDefaultFeedForTesting(brave_news::mojom::PublisherPtr publisher);

  void AddObserver(PageFeedsObserver* observer);
  void RemoveObserver(PageFeedsObserver* observer);

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void DOMContentLoaded(content::RenderFrameHost* rfh) override;

  // brave_news::BraveNewsPrefManager::PrefObserver:
  void OnPublishersChanged() override;

 private:
  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // Forgets the feeds of the page that was shown.
  void ResetPageFeeds();

  bool ShouldFindFeeds();
  void OnReceivedNewPublishers(brave_news::MojomPublishers publishers);
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

  ui::ScopedUnownedUserData<BraveNewsTabHelper> scoped_unowned_user_data_;

  base::WeakPtrFactory<BraveNewsTabHelper> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_TAB_HELPER_H_
