// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_tab_helper.h"

#include <dirent.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/containers/flat_map.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/task_traits_extension.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-params-data.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "chrome/browser/feed/rss_links_fetcher.h"
#include "components/feed/buildflags.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
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

bool BraveNewsTabHelper::is_subscribed(const FeedDetails& feed_details) {
  // This must be a direct feed that we aren't subscribed to.
  auto publisher = controller_->publisher_controller()->GetPublisherForFeed(
      feed_details.feed_url);
  if (!publisher) {
    publisher = controller_->publisher_controller()->GetPublisherById(
        feed_details.publisher_id);
  }
  return brave_news::IsPublisherEnabled(publisher);
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
  auto publisher = controller_->publisher_controller()->GetPublisherById(
      feed_details.publisher_id);
  if (publisher) {
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
    auto publisher = controller_->publisher_controller()->GetPublisherForFeed(
        feed->feed_url);
    auto publisher_id = publisher ? publisher->publisher_id : "";
    available_feeds_.push_back(
        {feed->feed_url, publisher_id, feed->feed_title});
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
    observer->OnAvailableFeedsChanged(available_feeds());
}

void BraveNewsTabHelper::PrimaryPageChanged(content::Page& page) {
  // Invalidate all weak pointers - we're on a new page now.
  weak_ptr_factory_.InvalidateWeakPtrs();

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
  auto callback = base::BindOnce(&BraveNewsTabHelper::OnReceivedRssUrls,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 contents->GetLastCommittedURL());
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&feed::FetchRssLinks, contents->GetLastCommittedURL(),
                     contents, std::move(callback)));
#endif

  AvailableFeedsChanged();
}

void BraveNewsTabHelper::OnPublishersUpdated(
    brave_news::PublishersController*) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AvailableFeedsChanged();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
