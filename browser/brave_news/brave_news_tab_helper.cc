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
#include "base/ranges/algorithm.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
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
  CHECK(!contents->GetBrowserContext()->IsOffTheRecord());

  pref_observation_.Observe(controller_->prefs());
  controller_->GetPublishers(base::DoNothing());
}

BraveNewsTabHelper::~BraveNewsTabHelper() = default;

const std::vector<GURL> BraveNewsTabHelper::GetAvailableFeedUrls() {
  std::vector<GURL> feeds;
  base::flat_set<GURL> seen_feeds;

  auto current_url = GetWebContents().GetLastCommittedURL();
  if (!current_url.is_empty() && !current_url.host().empty()) {
    auto* default_publisher =
        controller_->publisher_controller()->GetPublisherForSite(current_url);

    if (default_publisher) {
      seen_feeds.insert(default_publisher->feed_source);
      feeds.push_back(default_publisher->feed_source);
    }
  }

  for (const auto& rss_feed : rss_page_feeds_) {
    if (base::Contains(seen_feeds, rss_feed.feed_url)) {
      continue;
    }

    seen_feeds.insert(rss_feed.feed_url);
    feeds.push_back(rss_feed.feed_url);
  }

  return feeds;
}

bool BraveNewsTabHelper::IsSubscribed(const GURL& feed_url) {
  auto* publisher =
      controller_->publisher_controller()->GetPublisherForFeed(feed_url);
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
  return base::ranges::any_of(GetAvailableFeedUrls(), [this](const auto& feed) {
    return IsSubscribed(feed);
  });
}

std::string BraveNewsTabHelper::GetTitleForFeedUrl(const GURL& feed_url) {
  // If there's a default publisher for this |feed_url| we should use it's
  // title.
  if (auto* default_publisher =
          controller_->publisher_controller()->GetPublisherForFeed(feed_url)) {
    return default_publisher->publisher_name;
  }

  // Otherwise, find the |FeedDetails| for this |feed_url|.
  auto it =
      base::ranges::find(rss_page_feeds_, feed_url, &FeedDetails::feed_url);
  // If we don't have any details, return the empty string as we don't know what
  // the title should be.
  if (it == rss_page_feeds_.end()) {
    return "";
  }

  // Return our cached title, if we have one.
  if (!it->title.empty()) {
    return it->title;
  }

  // If we have an entry, but don't have a title for it, we should request the
  // feed (if we haven't already). Observers will be notified once the feed
  // resolves and we extract a title.
  if (!it->requested_feed) {
    const auto url = web_contents()->GetLastCommittedURL();
    controller_->FindFeeds(
        feed_url,
        base::BindOnce(&BraveNewsTabHelper::OnFoundFeedData,
                       weak_ptr_factory_.GetWeakPtr(), feed_url, url));
    it->requested_feed = true;
  }

  // In the meantime, use the feed_url as the title. Observers will be
  // notified when we receive a title.
  return feed_url.spec();
}

void BraveNewsTabHelper::ToggleSubscription(const GURL& feed_url) {
  bool subscribed = IsSubscribed(feed_url);
  auto* publisher =
      controller_->publisher_controller()->GetPublisherForFeed(feed_url);
  if (publisher) {
    controller_->SetPublisherPref(
        publisher->publisher_id, subscribed
                                     ? brave_news::mojom::UserEnabled::DISABLED
                                     : brave_news::mojom::UserEnabled::ENABLED);
  } else if (!subscribed) {
    controller_->SubscribeToNewDirectFeed(feed_url, base::DoNothing());
  }
}

void BraveNewsTabHelper::OnReceivedRssUrls(const GURL& site_url,
                                           std::vector<GURL> feed_urls) {
  if (site_url != GetWebContents().GetLastCommittedURL()) {
    return;
  }

  // First things first, we just store the urls. This let's us know that we have
  // feeds, so we should show the button.
  for (const auto& url : feed_urls) {
    rss_page_feeds_.push_back({.feed_url = url, .title = ""});
  }

  AvailableFeedsChanged();
}

void BraveNewsTabHelper::OnFoundFeedData(
    const GURL& feed_url,
    const GURL& site_url,
    std::vector<brave_news::mojom::FeedSearchResultItemPtr> feeds) {
  if (site_url != GetWebContents().GetLastCommittedURL()) {
    return;
  }

  if (feeds.empty()) {
    rss_page_feeds_.erase(
        base::ranges::remove(rss_page_feeds_, feed_url, &FeedDetails::feed_url),
        rss_page_feeds_.end());
  } else {
    DCHECK_EQ(1u, feeds.size())
        << "As we were passed a FeedURL, this should only find one feed.";

    // Find the FeedDetails this title is for.
    auto result = base::ranges::find_if(
        rss_page_feeds_,
        [feed_url](const auto& detail) { return detail.feed_url == feed_url; });

    // If there was a match, set the title.
    if (result != rss_page_feeds_.end()) {
      result->title = feeds.at(0)->feed_title;
    }
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

void BraveNewsTabHelper::OnReceivedNewPublishers(
    brave_news::Publishers publishers) {
  AvailableFeedsChanged();
}

void BraveNewsTabHelper::AvailableFeedsChanged() {
  const auto& feed_urls = GetAvailableFeedUrls();
  for (auto& observer : observers_) {
    observer.OnAvailableFeedsChanged(feed_urls);
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

void BraveNewsTabHelper::OnPublishersChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  controller_->GetPublishers(
      base::BindOnce(&BraveNewsTabHelper::OnReceivedNewPublishers,
                     weak_ptr_factory_.GetWeakPtr()));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
