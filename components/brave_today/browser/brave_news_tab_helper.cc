// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_tab_helper.h"
#include <algorithm>
#include <string>
#include <vector>
#include "absl/types/optional.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "chrome/browser/feed/rss_links_fetcher.h"
#include "components/feed/buildflags.h"
#include "content/public/browser/page.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

BraveNewsTabHelper::BraveNewsTabHelper(content::WebContents* contents)
    : content::WebContentsUserData<BraveNewsTabHelper>(*contents),
      content::WebContentsObserver(contents),
      controller_(
          brave_news::BraveNewsControllerFactory::GetControllerForContext(
              contents->GetBrowserContext())) {}

BraveNewsTabHelper::~BraveNewsTabHelper() = default;

bool BraveNewsTabHelper::is_subscribed(const FeedDetails& feed_details) {
  return false;
}

bool BraveNewsTabHelper::is_subscribed() {
  for (const auto& feed : available_feeds_) {
    if (is_subscribed(feed))
      return true;
  }
  return false;
}

void BraveNewsTabHelper::ToggleSubscription(const FeedDetails& feed_details) {
  bool subscribed = is_subscribed(feed_details);
  if (!feed_details.publisher_id.empty()) {
    controller_->SetPublisherPref(
        feed_details.publisher_id,
        subscribed ? brave_news::mojom::UserEnabled::DISABLED
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
    available_feeds_.push_back({feed->feed_url, "", feed->feed_title});
  }

  AvailableFeedsChanged();
}

void BraveNewsTabHelper::AddObserver(PageFeedsObserver* observer) {
  observers_.push_back(observer);
}

void BraveNewsTabHelper::RemoveObserver(PageFeedsObserver* observer) {
  std::ignore = std::remove(observers_.begin(), observers_.end(), observer);
}

void BraveNewsTabHelper::AvailableFeedsChanged() {
  for (auto* observer : observers_)
    observer->OnAvailableFeedsChanged(available_feeds_);

  for (const auto& feed : available_feeds_)
    LOG(ERROR) << "Feed: " << feed.title << ", URL: " << feed.feed_url
               << ", Id: " << feed.publisher_id;
}

void BraveNewsTabHelper::PrimaryPageChanged(content::Page& page) {
  available_feeds_.clear();

  auto* contents =
      content::WebContents::FromRenderFrameHost(&page.GetMainDocument());

  auto default_publisher =
      controller_->publisher_controller()->GetPublisherForSite(
          contents->GetLastCommittedURL());
  if (default_publisher) {
    available_feeds_.push_back({default_publisher->feed_source,
                                default_publisher->publisher_id,
                                default_publisher->publisher_name});
  }

#if BUILDFLAG(ENABLE_FEED_V2)
  feed::FetchRssLinks(contents->GetLastCommittedURL(), contents,
                      base::BindOnce(&BraveNewsTabHelper::OnReceivedRssUrls,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     contents->GetLastCommittedURL()));
#endif

  AvailableFeedsChanged();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
