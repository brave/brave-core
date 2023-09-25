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

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "chrome/browser/feed/rss_links_fetcher.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feed/buildflags.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// static
void BraveNewsTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (contents->GetBrowserContext()->IsOffTheRecord()) {
    return;
  }

  CreateForWebContents(contents);
}

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

  auto current_url = GetWebContents().GetLastCommittedURL();
  if (!current_url.is_empty() && !current_url.host().empty()) {
    auto* default_publisher =
        controller_->publisher_controller()->GetPublisherForSite(current_url);

    if (default_publisher) {
      seen_feeds.insert(default_publisher->feed_source);
      feeds.push_back(
          {default_publisher->feed_source, default_publisher->publisher_name});
    }
  }

  for (const auto& rss_feed : rss_page_feeds_) {
    if (base::Contains(seen_feeds, rss_feed.feed_url)) {
      continue;
    }

    seen_feeds.insert(rss_feed.feed_url);
    feeds.push_back(rss_feed);
  }

  return feeds;
}

bool BraveNewsTabHelper::IsSubscribed(const FeedDetails& feed_details) {
  auto* publisher = controller_->publisher_controller()->GetPublisherForFeed(
      feed_details.feed_url);
  if (!publisher) {
    return false;
  }

  // When a direct feed exists, it is always subscribed (there is no way to
  // have an unsubscribed direct feed).
  if (publisher->type == brave_news::mojom::PublisherType::DIRECT_SOURCE) {
    return true;
  }

  // Otherwise, it's a combined feed, so just return whether the user has
  // enabled it.
  return publisher->user_enabled_status ==
         brave_news::mojom::UserEnabled::ENABLED;
}

bool BraveNewsTabHelper::IsSubscribed() {
  for (const auto& feed : GetAvailableFeeds()) {
    if (IsSubscribed(feed)) {
      return true;
    }
  }
  return false;
}

void BraveNewsTabHelper::ToggleSubscription(const FeedDetails& feed_details) {
  bool subscribed = IsSubscribed(feed_details);
  auto* publisher = controller_->publisher_controller()->GetPublisherForFeed(
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
  if (site_url != GetWebContents().GetLastCommittedURL()) {
    return;
  }

  for (const auto& url : feed_urls) {
    controller_->FindFeeds(
        url, base::BindOnce(&BraveNewsTabHelper::OnFoundFeeds,
                            weak_ptr_factory_.GetWeakPtr(), site_url));
  }
}

void BraveNewsTabHelper::OnFoundFeeds(
    const GURL& site_url,
    std::vector<brave_news::mojom::FeedSearchResultItemPtr> feeds) {
  if (site_url != GetWebContents().GetLastCommittedURL()) {
    return;
  }

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

bool BraveNewsTabHelper::ShouldFindFeeds() {
  auto* prefs = Profile::FromBrowserContext(web_contents()->GetBrowserContext())
                    ->GetPrefs();
  return brave_news::GetIsEnabled(prefs) &&
         prefs->GetBoolean(brave_news::prefs::kShouldShowToolbarButton);
}

void BraveNewsTabHelper::AvailableFeedsChanged() {
  for (auto& observer : observers_) {
    observer.OnAvailableFeedsChanged(GetAvailableFeeds());
  }
}

void BraveNewsTabHelper::PrimaryPageChanged(content::Page& page) {
  // Invalidate all weak pointers - we're on a new page now.
  weak_ptr_factory_.InvalidateWeakPtrs();
  rss_page_feeds_.clear();
  AvailableFeedsChanged();
}

void BraveNewsTabHelper::DOMContentLoaded(content::RenderFrameHost* rfh) {
  if (GetWebContents().GetPrimaryMainFrame() != rfh) {
    return;
  }

  if (!ShouldFindFeeds()) {
    return;
  }

  feed::FetchRssLinks(GetWebContents().GetLastCommittedURL(), &GetWebContents(),
                      base::BindOnce(&BraveNewsTabHelper::OnReceivedRssUrls,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     GetWebContents().GetLastCommittedURL()));
}

void BraveNewsTabHelper::OnPublishersUpdated(
    brave_news::PublishersController*) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AvailableFeedsChanged();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
