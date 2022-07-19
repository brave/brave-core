// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_tab_helper.h"

#include <dirent.h>
#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
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
  controller_->publisher_controller()->AddObserver(this);
  controller_->GetPublishers(base::DoNothing());
}

BraveNewsTabHelper::~BraveNewsTabHelper() {
  controller_->publisher_controller()->RemoveObserver(this);
}

std::vector<BraveNewsTabHelper::FeedDetails>
BraveNewsTabHelper::GetAvailableFeeds() {
  std::vector<FeedDetails> feeds;

  std::unordered_set<std::string> seen_feeds;
  auto default_publisher =
      controller_->publisher_controller()->GetPublisherForSite(
          GetWebContents().GetLastCommittedURL());
  if (default_publisher) {
    seen_feeds.insert(default_publisher->feed_source.spec());
    feeds.insert(feeds.begin(), {default_publisher->feed_source,
                                 default_publisher->publisher_name});
  }

  for (const auto& rss_feed : rss_page_feeds_) {
    auto url = rss_feed.feed_url.spec();
    if (seen_feeds.find(url) != seen_feeds.end())
      continue;

    seen_feeds.insert(url);
    feeds.push_back(rss_feed);
  }

  return feeds;
}

bool BraveNewsTabHelper::is_subscribed(const FeedDetails& feed_details) {
  auto publisher = controller_->publisher_controller()->GetPublisherForFeed(
      feed_details.feed_url);
  return brave_news::IsPublisherEnabled(publisher);
}

bool BraveNewsTabHelper::is_subscribed() {
  for (const auto& feed : GetAvailableFeeds()) {
    if (is_subscribed(feed))
      return true;
  }
  return false;
}

void BraveNewsTabHelper::ToggleSubscription(const FeedDetails& feed_details) {
  bool subscribed = is_subscribed(feed_details);
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
  observers_.push_back(observer);
}

void BraveNewsTabHelper::RemoveObserver(PageFeedsObserver* observer) {
  observers_.erase(std::find(observers_.begin(), observers_.end(), observer));
}

void BraveNewsTabHelper::AvailableFeedsChanged() {
  for (auto* observer : observers_)
    observer->OnAvailableFeedsChanged(GetAvailableFeeds());
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
