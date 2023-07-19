// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_fetcher.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/combined_feed_parsing.h"
#include "brave/components/brave_news/browser/feed_controller.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_private_cdn/headers.h"

namespace brave_news {

namespace {

const char kEtagHeaderKey[] = "etag";

GURL GetFeedUrl(const std::string& locale) {
  GURL feed_url("https://" + brave_news::GetHostname() + "/brave-today/feed." +
                locale + "json");
  return feed_url;
}

}  // namespace

FeedFetcher::FeedSourceResult::FeedSourceResult() = default;
FeedFetcher::FeedSourceResult::FeedSourceResult(std::string key,
                                                std::string etag,
                                                FeedItems items)
    : key(std::move(key)), etag(std::move(etag)), items(std::move(items)) {}
FeedFetcher::FeedSourceResult::~FeedSourceResult() = default;
FeedFetcher::FeedSourceResult::FeedSourceResult(FeedFetcher::FeedSourceResult&&) = default;

FeedFetcher::FeedFetcher(
    PublishersController& publishers_controller,
    ChannelsController& channels_controller,
    api_request_helper::APIRequestHelper& api_request_helper)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      api_request_helper_(api_request_helper) {}

FeedFetcher::~FeedFetcher() = default;

void FeedFetcher::FetchFeed(FetchFeedCallback callback) {
  VLOG(1) << __FUNCTION__;

  publishers_controller_->GetOrFetchPublishers(
      base::BindOnce(&FeedFetcher::OnFetchFeedFetchedPublishers,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FeedFetcher::OnFetchFeedFetchedPublishers(FetchFeedCallback callback,
                                               Publishers publishers) {
  if (publishers.empty()) {
    LOG(ERROR) << "Brave News Publisher list was empty";
    std::move(callback).Run({}, {});
    return;
  }

  auto locales = GetMinimalLocalesSet(channels_controller_->GetChannelLocales(),
                                      publishers);
  auto downloaded_callback = base::BarrierCallback<FeedSourceResult>(
      locales.size(),
      base::BindOnce(&FeedFetcher::OnFetchFeedFetchedAll,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(publishers)));

  for (const auto& locale : locales) {
    GURL feed_url(GetFeedUrl(locale));
    VLOG(1) << "Making feed request to " << feed_url.spec();
    api_request_helper_->Request(
        "GET", feed_url, "", "",
        base::BindOnce(&FeedFetcher::OnFetchFeedFetchedFeed,
                       weak_ptr_factory_.GetWeakPtr(), locale,
                       downloaded_callback));
  }
}

void FeedFetcher::OnFetchFeedFetchedFeed(
    std::string locale,
    FetchFeedSourceCallback callback,
    api_request_helper::APIRequestResult result) {
  std::string etag;
  if (result.headers().contains(kEtagHeaderKey)) {
    etag = result.headers().at(kEtagHeaderKey);
  }

  VLOG(1) << "Downloaded feed, status: " << result.response_code()
          << " etag: " << etag;

  // Handle bad response
  if (result.response_code() != 200 || result.value_body().is_none()) {
    LOG(ERROR) << "Bad response from brave news feed.json. Status: "
               << result.response_code();
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run({locale, etag, ParseFeedItems(result.value_body())});
}

void FeedFetcher::OnFetchFeedFetchedAll(FetchFeedCallback callback,
                                        Publishers publishers,
                                        std::vector<FeedSourceResult> results) {
  std::size_t total_size = 0;
  for (const auto& result : results) {
    total_size += result.items.size();
  }
  VLOG(1) << "All feed item fetches done with item count: " << total_size;

  ETags etags;
  FeedItems feed;
  feed.reserve(total_size);
  for (auto& result : results) {
    etags[result.key] = result.etag;
    feed.insert(feed.end(), std::make_move_iterator(result.items.begin()),
                std::make_move_iterator(result.items.end()));
  }

  std::move(callback).Run(std::move(feed), std::move(etags));
}

void FeedFetcher::IsUpdateAvailable(ETags etags,
                                    UpdateAvailableCallback callback) {
  VLOG(1) << __FUNCTION__;

  publishers_controller_->GetOrFetchPublishers(base::BindOnce(
      &FeedFetcher::OnIsUpdateAvailableFetchedPublishers,
      weak_ptr_factory_.GetWeakPtr(), std::move(etags), std::move(callback)));
}

void FeedFetcher::OnIsUpdateAvailableFetchedPublishers(
    ETags etags,
    UpdateAvailableCallback callback,
    Publishers publishers) {
  auto locales = GetMinimalLocalesSet(channels_controller_->GetChannelLocales(),
                                      publishers);
  VLOG(1) << __FUNCTION__ << " - going to fetch feed items for "
          << locales.size() << " locales.";
  auto check_completed_callback = base::BarrierCallback<bool>(
      locales.size(),
      base::BindOnce(&FeedFetcher::OnIsUpdateAvailableCheckedFeeds,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));

  for (const auto& locale : locales) {
    auto it = etags.find(locale);
    // If we haven't fetched this feed yet, we need to update it.
    if (it == etags.end()) {
      check_completed_callback.Run(true);
      continue;
    }

    // Get new Etag
    api_request_helper_->Request(
        "HEAD", GetFeedUrl(locale), "", "",
        base::BindOnce(&FeedFetcher::OnIsUpdateAvailableFetchedHead,
                       weak_ptr_factory_.GetWeakPtr(), it->second,
                       check_completed_callback),
        brave::private_cdn_headers, {.auto_retry_on_network_change = true});
  }
}

void FeedFetcher::OnIsUpdateAvailableFetchedHead(
    std::string current_etag,
    base::RepeatingCallback<void(bool)> has_update_callback,
    api_request_helper::APIRequestResult result) {
  std::string etag;
  if (result.headers().contains(kEtagHeaderKey)) {
    etag = result.headers().at(kEtagHeaderKey);
  }
  // Empty etag means perhaps server isn't supporting
  // the header right now, so we assume we should
  // always fetch the body at these times.
  if (etag.empty()) {
    LOG(ERROR) << "Brave News did not get correct etag, "
                  "therefore assuming etags aren't working and feed "
                  "changed.";
    has_update_callback.Run(true);
    return;
  }
  VLOG(1) << "Comparing feed etag - "
             "Original: "
          << current_etag << " Remote: " << etag;
  // Compare remote etag with last feed fetch.
  if (current_etag == etag) {
    // Nothing to do
    has_update_callback.Run(false);
    return;
  }

  // Needs update
  has_update_callback.Run(true);
}

void FeedFetcher::OnIsUpdateAvailableCheckedFeeds(
    UpdateAvailableCallback callback,
    std::vector<bool> has_updates) {
  std::move(callback).Run(base::ranges::any_of(
      has_updates, [](bool has_update) { return has_update; }));
}

}  // namespace brave_news
