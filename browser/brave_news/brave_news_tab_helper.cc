// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_tab_helper.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
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
#include "url/gurl.h"

BraveNewsTabHelper::FeedDetails::FeedDetails() = default;
BraveNewsTabHelper::FeedDetails::~FeedDetails() = default;
BraveNewsTabHelper::FeedDetails::FeedDetails(FeedDetails&&) = default;
BraveNewsTabHelper::FeedDetails& BraveNewsTabHelper::FeedDetails::operator=(
    BraveNewsTabHelper::FeedDetails&&) = default;

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
      controller_(brave_news::BraveNewsControllerFactory::GetForBrowserContext(
          contents->GetBrowserContext())) {
  CHECK(!contents->GetBrowserContext()->IsOffTheRecord());

  pref_observation_.Observe(&controller_->prefs());
  UpdatePageFeed();
}

BraveNewsTabHelper::~BraveNewsTabHelper() = default;

const std::vector<GURL> BraveNewsTabHelper::GetAvailableFeedUrls() {
  std::vector<GURL> feeds;
  base::flat_set<GURL> seen_feeds;

  if (default_feed_) {
    seen_feeds.insert(default_feed_->feed_source);
    feeds.push_back(default_feed_->feed_source);
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
  if (default_feed_ && default_feed_->feed_source == feed_url) {
    return brave_news::IsSubscribed(default_feed_);
  }

  auto it = std::ranges::find(rss_page_feeds_, feed_url,
                              [](const auto& feed) { return feed.feed_url; });
  if (it == rss_page_feeds_.end()) {
    return false;
  }

  return it->subscribed;
}

bool BraveNewsTabHelper::IsSubscribed() {
  return (default_feed_ && brave_news::IsSubscribed(default_feed_)) ||
         base::ranges::any_of(rss_page_feeds_,
                              [](const auto& feed) { return feed.subscribed; });
}

std::string BraveNewsTabHelper::GetTitleForFeedUrl(const GURL& feed_url) {
  // If there's a default publisher for this |feed_url| we should use it's
  // title.
  if (default_feed_ && default_feed_->feed_source == feed_url) {
    return default_feed_->publisher_name;
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
  controller_->GetPublisherForFeed(
      feed_url,
      base::BindOnce(
          [](base::WeakPtr<BraveNewsTabHelper> tab_helper, bool subscribed,
             GURL feed_url, brave_news::mojom::PublisherPtr publisher) {
            if (!tab_helper) {
              return;
            }

            if (publisher) {
              tab_helper->controller_->SetPublisherPref(
                  publisher->publisher_id,
                  subscribed ? brave_news::mojom::UserEnabled::DISABLED
                             : brave_news::mojom::UserEnabled::ENABLED);
            } else {
              tab_helper->controller_->SubscribeToNewDirectFeed(
                  feed_url, base::DoNothing());
            }

            tab_helper->OnPublishersChanged();
          },
          weak_ptr_factory_.GetWeakPtr(), subscribed, feed_url));
}

void BraveNewsTabHelper::OnReceivedRssUrls(const GURL& site_url,
                                           std::vector<GURL> feed_urls) {
  if (site_url != GetWebContents().GetLastCommittedURL()) {
    return;
  }

  // For each feed, we need to check if its a combined feed.
  for (const auto& feed_url : feed_urls) {
    controller_->GetPublisherForFeed(
        feed_url,
        base::BindOnce(
            [](base::WeakPtr<BraveNewsTabHelper> tab_helper, GURL feed_url,
               brave_news::mojom::PublisherPtr publisher) {
              if (!tab_helper) {
                return;
              }

              FeedDetails feed;
              feed.feed_url = feed_url;
              if (publisher) {
                if (publisher->type !=
                    brave_news::mojom::PublisherType::DIRECT_SOURCE) {
                  feed.combined_publisher_id = publisher->publisher_id;
                }
                feed.title = publisher->publisher_name;
                feed.requested_feed = true;
              }
              tab_helper->rss_page_feeds_.push_back(std::move(feed));
              tab_helper->OnPublishersChanged();
            },
            weak_ptr_factory_.GetWeakPtr(), feed_url));
  }
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
  return brave_news::IsEnabled(prefs) &&
         prefs->GetBoolean(brave_news::prefs::kShouldShowToolbarButton);
}

void BraveNewsTabHelper::OnReceivedNewPublishers(
    brave_news::Publishers publishers) {
  UpdatePageFeed();
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
  default_feed_ = nullptr;

  UpdatePageFeed();
  AvailableFeedsChanged();
}

void BraveNewsTabHelper::UpdatePageFeed() {
  controller_->GetPublisherForSite(
      GetWebContents().GetLastCommittedURL(),
      base::BindOnce(
          [](base::WeakPtr<BraveNewsTabHelper> tab_helper,
             brave_news::mojom::PublisherPtr publisher) {
            if (!tab_helper) {
              return;
            }

            if (!publisher) {
              tab_helper->default_feed_ = nullptr;
              tab_helper->AvailableFeedsChanged();
              return;
            }

            tab_helper->default_feed_ = std::move(publisher);
            tab_helper->AvailableFeedsChanged();
          },
          weak_ptr_factory_.GetWeakPtr()));
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
  auto subscriptions = controller_->prefs().GetSubscriptions();
  for (auto& feed : rss_page_feeds_) {
    feed.subscribed =
        base::Contains(subscriptions.enabled_publishers(),
                       feed.combined_publisher_id) ||
        base::Contains(subscriptions.direct_feeds(), feed.feed_url,
                       [](const auto& direct_feed) { return direct_feed.url; });
  }
  controller_->GetPublishers(
      base::BindOnce(&BraveNewsTabHelper::OnReceivedNewPublishers,
                     weak_ptr_factory_.GetWeakPtr()));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
