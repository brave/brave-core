// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_controller.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/uuid.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_news/browser/html_parsing.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "url/gurl.h"

namespace brave_news {

DirectFeedController::FindFeedRequest::FindFeedRequest(
    const GURL& possible_feed_or_site_url,
    mojom::BraveNewsController::FindFeedsCallback callback)
    : possible_feed_or_site_url(possible_feed_or_site_url),
      callback(std::move(callback)) {}

DirectFeedController::FindFeedRequest::FindFeedRequest(
    DirectFeedController::FindFeedRequest&&) = default;
DirectFeedController::FindFeedRequest&
DirectFeedController::FindFeedRequest::operator=(
    DirectFeedController::FindFeedRequest&&) = default;

DirectFeedController::FindFeedRequest::~FindFeedRequest() = default;

DirectFeedController::DirectFeedController(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), fetcher_(url_loader_factory) {}

DirectFeedController::~DirectFeedController() = default;

bool DirectFeedController::AddDirectFeedPref(
    const GURL& feed_url,
    const std::string& title,
    const std::optional<std::string>& id) {
  // Check if feed url already exists
  const auto& existing_feeds = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  for (const auto&& [key, value] : existing_feeds) {
    // Non dict values will be flagged as an issue elsewhere.
    if (!value.is_dict()) {
      continue;
    }

    const auto* existing_url =
        value.GetDict().FindString(prefs::kBraveNewsDirectFeedsKeySource);
    if (GURL(*existing_url) == feed_url.spec()) {
      // It's a duplicate.
      return false;
    }
  }

  // Feed is valid, we can add the url now
  // UUID for each entry as feed url might change via redirects etc
  auto entry_id =
      id.value_or(base::Uuid::GenerateRandomV4().AsLowercaseString());
  std::string entry_title = title.empty() ? feed_url.spec() : title;

  // We use a dictionary pref, but that's to reserve space for more
  // future customization on a feed. For now we just store a bool, and
  // remove the entire entry if a user unsubscribes from a user feed.
  ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsDirectFeeds);
  base::Value::Dict value;
  value.Set(prefs::kBraveNewsDirectFeedsKeySource, feed_url.spec());
  value.Set(prefs::kBraveNewsDirectFeedsKeyTitle, entry_title);
  update->SetByDottedPath(entry_id, std::move(value));

  return true;
}

void DirectFeedController::RemoveDirectFeedPref(
    const std::string& publisher_id) {
  ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsDirectFeeds);
  update->Remove(publisher_id);
}

std::vector<mojom::PublisherPtr> DirectFeedController::ParseDirectFeedsPref() {
  std::vector<mojom::PublisherPtr> result;
  const auto& pref = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  ParseDirectPublisherList(pref, &result);
  return result;
}

void DirectFeedController::FindFeeds(
    const GURL& possible_feed_or_site_url,
    mojom::BraveNewsController::FindFeedsCallback callback) {
  CHECK(possible_feed_or_site_url.is_valid() &&
        !possible_feed_or_site_url.is_empty());

  if (ongoing_requests_.count(possible_feed_or_site_url)) {
    DVLOG(2) << "Accumulated: " << possible_feed_or_site_url.spec();
    ongoing_requests_[possible_feed_or_site_url].push_back(
        {possible_feed_or_site_url, std::move(callback)});
    return;
  }

  if (ongoing_requests_.size() >= kMaxOngoingRequests) {
    DVLOG(2) << "Queued: " << possible_feed_or_site_url.spec();
    pending_requests_.push({possible_feed_or_site_url, std::move(callback)});
    return;
  }

  DVLOG(2) << "Kick off: " << possible_feed_or_site_url.spec();
  ongoing_requests_[possible_feed_or_site_url].push_back(
      {possible_feed_or_site_url, std::move(callback)});
  FindFeedsImpl(possible_feed_or_site_url);
}

void DirectFeedController::FindFeedsImpl(
    const GURL& possible_feed_or_site_url) {
  DVLOG(2) << __FUNCTION__ << " " << possible_feed_or_site_url.spec();
  fetcher_.DownloadFeed(
      possible_feed_or_site_url, "",
      base::BindOnce(&DirectFeedController::OnFindFeedsImplDownloadedFeed,
                     weak_ptr_factory_.GetWeakPtr(),
                     possible_feed_or_site_url));
}

void DirectFeedController::OnFindFeedsImplDownloadedFeed(
    const GURL& feed_url,
    DirectFeedResponse response) {
  if (auto* feed = absl::get_if<DirectFeedResult>(&response.result)) {
    std::vector<mojom::FeedSearchResultItemPtr> results;

    auto feed_result = mojom::FeedSearchResultItem::New();
    feed_result->feed_title = (std::string)feed->title;
    feed_result->feed_url = feed_url;
    results.emplace_back(std::move(feed_result));
    OnFindFeedsImplResponse(feed_url, std::move(results));
    return;
  }

  if (!response.mime_type.empty() &&
      response.mime_type.find("html") != std::string::npos) {
    auto& body_content =
        absl::get<DirectFeedError>(response.result).body_content;
    VLOG(1) << "Had html type";
    // Get feed links from doc
    auto feed_urls = GetFeedURLsFromHTMLDocument(response.charset, body_content,
                                                 response.final_url);
    auto all_done_handler = base::BindOnce(
        [](mojom::BraveNewsController::FindFeedsCallback callback,
           std::vector<DirectFeedResponse> responses) {
          std::vector<mojom::FeedSearchResultItemPtr> results;
          for (const auto& response : responses) {
            if (auto* feed = absl::get_if<DirectFeedResult>(&response.result)) {
              if (feed->title.empty()) {
                continue;
              }
              auto feed_result = mojom::FeedSearchResultItem::New();
              feed_result->feed_title = (std::string)feed->title;
              feed_result->feed_url = response.url;
              results.emplace_back(std::move(feed_result));
            }
          }
          VLOG(1) << "Valid feeds found via HTML content: " << results.size();
          std::move(callback).Run(std::move(results));
        },
        base::BindOnce(&DirectFeedController::OnFindFeedsImplResponse,
                       weak_ptr_factory_.GetWeakPtr(), feed_url));
    VLOG(1) << "Feed URLs found in HTML content: " << feed_urls.size();
    auto feed_handler = base::BarrierCallback<DirectFeedResponse>(
        feed_urls.size(), std::move(all_done_handler));
    for (auto& url : feed_urls) {
      fetcher_.DownloadFeed(url, "", feed_handler);
    }
    return;
  }

  // If we didn't get a valid response, call back with no results.
  OnFindFeedsImplResponse(feed_url, {});
}

void DirectFeedController::OnFindFeedsImplResponse(
    const GURL& feed_url,
    std::vector<mojom::FeedSearchResultItemPtr> results) {
  if (ongoing_requests_.count(feed_url)) {
    if (ongoing_requests_[feed_url].size() == 1u) {
      std::move(ongoing_requests_[feed_url].front().callback)
          .Run(std::move(results));
    } else {
      // We need to do deep-copy
      for (auto& request : ongoing_requests_[feed_url]) {
        std::vector<mojom::FeedSearchResultItemPtr> clone;
        base::ranges::transform(results, std::back_inserter(clone),
                                &mojom::FeedSearchResultItemPtr::Clone);
        std::move(request.callback).Run(std::move(clone));
      }
    }

    ongoing_requests_.erase(feed_url);
  }

  DVLOG(2) << ongoing_requests_.size();

  if (ongoing_requests_.size() >= kMaxOngoingRequests ||
      pending_requests_.empty()) {
    return;
  }

  auto request = std::move(pending_requests_.front());
  pending_requests_.pop();

  auto target_url = request.possible_feed_or_site_url;
  auto& requests = ongoing_requests_[target_url];
  requests.push_back(std::move(request));
  if (requests.size() == 1) {
    FindFeedsImpl(target_url);
  }
}

void DirectFeedController::VerifyFeedUrl(const GURL& feed_url,
                                         IsValidCallback callback) {
  // Download the feed and once it's done, see if there's any content.
  // This verifies that the URL is reachable, that it has content,
  // and that the content has the correct fields for Brave News.
  // TODO(petemill): Cache for a certain amount of time since user
  // will likely add to their user feed sources. Unless this is already
  // cached via network service?
  fetcher_.DownloadFeed(
      feed_url, "",
      base::BindOnce(
          [](IsValidCallback callback, DirectFeedResponse response) {
            // Handle response
            std::string title = "";
            bool success = false;
            if (auto* feed = absl::get_if<DirectFeedResult>(&response.result)) {
              title = feed->title.c_str();
              success = true;
            }
            std::move(callback).Run(success, title);
          },
          std::move(callback)));
}

}  // namespace brave_news
