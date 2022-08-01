// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_tab_helper.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-params-data.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "chrome/browser/feed/rss_links_fetcher.h"
#include "components/feed/buildflags.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

BraveNewsTabHelper::BraveNewsTabHelper(content::WebContents* contents)
    : content::WebContentsUserData<BraveNewsTabHelper>(*contents),
      content::WebContentsObserver(contents),
      controller_(
          brave_news::BraveNewsControllerFactory::GetControllerForContext(
              contents->GetBrowserContext())) {
  publishers_observation_.Observe(controller_->publisher_controller());
  controller_->GetPublishers(base::DoNothing());
}

BraveNewsTabHelper::~BraveNewsTabHelper() = default;

const std::vector<BraveNewsTabHelper::FeedDetails>
BraveNewsTabHelper::GetAvailableFeeds() {
  std::vector<FeedDetails> feeds;

  base::flat_set<GURL> seen_feeds;
  auto default_publisher =
      controller_->publisher_controller()->GetPublisherForSite(
          GetWebContents().GetLastCommittedURL());
  if (default_publisher) {
    seen_feeds.insert(default_publisher->feed_source);
    feeds.push_back(
        {default_publisher->feed_source, default_publisher->publisher_name});
  }

  for (const auto& rss_feed : rss_page_feeds_) {
    if (base::Contains(seen_feeds, rss_feed.feed_url))
      continue;

    seen_feeds.insert(rss_feed.feed_url);
    feeds.push_back(rss_feed);
  }

  return feeds;
}

bool BraveNewsTabHelper::IsSubscribed(const FeedDetails& feed_details) {
  auto publisher = controller_->publisher_controller()->GetPublisherForFeed(
      feed_details.feed_url);
  return brave_news::IsPublisherEnabled(publisher);
}

bool BraveNewsTabHelper::IsSubscribed() {
  for (const auto& feed : GetAvailableFeeds()) {
    if (IsSubscribed(feed))
      return true;
  }
  return false;
}

void BraveNewsTabHelper::ToggleSubscription(const FeedDetails& feed_details) {
  bool subscribed = IsSubscribed(feed_details);
  auto publisher = controller_->publisher_controller()->GetPublisherForFeed(
      feed_details.feed_url);
  if (publisher) {
    controller_->SetPublisherPref(
        publisher->publisher_id, subscribed
                                     ? brave_news::mojom::UserEnabled::DISABLED
                                     : brave_news::mojom::UserEnabled::ENABLED);
  } else if (!subscribed) {
    controller_->SubscribeToNewDirectFeed(feed_details.feed_url,
                                          base::DoNothing());
  }
}

void BraveNewsTabHelper::OnReceivedRssUrls(const GURL& site_url,
                                           std::vector<GURL> feed_urls) {
  if (site_url != GetWebContents().GetLastCommittedURL())
    return;

  for (const auto& url : feed_urls) {
    controller_->FindFeeds(
        url, base::BindOnce(&BraveNewsTabHelper::OnFoundFeeds,
                            weak_ptr_factory_.GetWeakPtr(), site_url));
  }
}

void BraveNewsTabHelper::OnFoundFeeds(
    const GURL& site_url,
    std::vector<brave_news::mojom::FeedSearchResultItemPtr> feeds) {
  if (site_url != GetWebContents().GetLastCommittedURL())
    return;

  for (const auto& feed : feeds) {
    rss_page_feeds_.push_back({feed->feed_url, feed->feed_title});
  }

  AvailableFeedsChanged();
}

void BraveNewsTabHelper::AddObserver(PageFeedsObserver* observer) {
  observers_.AddObserver(observer);
}

void BraveNewsTabHelper::RemoveObserver(PageFeedsObserver* observer) {
  observers_.RemoveObserver(observer);
}

void BraveNewsTabHelper::AvailableFeedsChanged() {
  for (auto& observer : observers_)
    observer.OnAvailableFeedsChanged(GetAvailableFeeds());
}

void BraveNewsTabHelper::PrimaryPageChanged(content::Page& page) {
  // Invalidate all weak pointers - we're on a new page now.
  weak_ptr_factory_.InvalidateWeakPtrs();

  rss_page_feeds_.clear();
  feed::FetchRssLinks(GetWebContents().GetLastCommittedURL(), &GetWebContents(),
                      base::BindOnce(&BraveNewsTabHelper::OnReceivedRssUrls,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     GetWebContents().GetLastCommittedURL()));

  AvailableFeedsChanged();
}

void BraveNewsTabHelper::OnPublishersUpdated(
    brave_news::PublishersController*) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AvailableFeedsChanged();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
