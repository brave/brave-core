// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/feed_controller.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/brave_today/browser/feed_building.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"

namespace brave_news {

namespace {

const char kEtagHeaderKey[] = "etag";

GURL GetFeedUrl() {
  GURL feed_url("https://" + brave_today::GetHostname() + "/brave-today/feed." +
                brave_today::GetRegionUrlPart() + "json");
  return feed_url;
}

}  // namespace

FeedController::FeedController(
    PublishersController* publishers_controller,
    history::HistoryService* history_service,
    api_request_helper::APIRequestHelper* api_request_helper)
    : publishers_controller_(publishers_controller),
      history_service_(history_service),
      api_request_helper_(api_request_helper),
      on_current_update_complete_(new base::OneShotEvent()),
      publishers_observation_(this) {
  publishers_observation_.Observe(publishers_controller);
}

FeedController::~FeedController() = default;

void FeedController::DoesFeedVersionDiffer(
    const std::string& matching_hash,
    mojom::BraveNewsController::IsFeedUpdateAvailableCallback callback) {
  GetOrFetchFeed(base::BindOnce(
      [](FeedController* controller, std::string matching_hash,
         mojom::BraveNewsController::IsFeedUpdateAvailableCallback callback) {
        VLOG(1) << "DoesFeedVersionMatch? " << matching_hash << " "
                << controller->current_feed_.hash;
        std::move(callback).Run(matching_hash !=
                                controller->current_feed_.hash);
      },
      base::Unretained(this), matching_hash, std::move(callback)));
}

void FeedController::GetOrFetchFeed(GetFeedCallback callback) {
  GetOrFetchFeed(base::BindOnce(
      [](FeedController* controller, GetFeedCallback callback) {
        if (!controller->current_feed_.hash.empty()) {
          auto clone = controller->current_feed_.Clone();
          std::move(callback).Run(std::move(clone));
          return;
        } else {
          // There was a problem fetching the feed.
          std::move(callback).Run(nullptr);
        }
      },
      base::Unretained(this), std::move(callback)));
}

void FeedController::EnsureFeedIsUpdating() {
  VLOG(1) << "EnsureFeedIsUpdating " << is_update_in_progress_;
  // Only 1 update at a time, other calls for data will wait for
  // the current operation via the `on_publishers_update_` OneShotEvent.
  if (is_update_in_progress_) {
    return;
  }
  is_update_in_progress_ = true;

  // Fetch https request via callback
  // TODO(petemill): avoid callback hell when c++ allows
  // us to do Promise.all and do call these 3 async functions
  // in parallel.
  auto onRequest = base::BindOnce(
      [](FeedController* controller, int status, const std::string& body,
         const base::flat_map<std::string, std::string>& headers) {
        std::string etag;
        if (headers.contains(kEtagHeaderKey)) {
          etag = headers.at(kEtagHeaderKey);
        }
        VLOG(1) << "Downloaded feed, status: " << status << " etag: " << etag;
        // Handle bad response
        if (status < 200 || status >= 300) {
          LOG(ERROR) << "Bad response from brave news feed.json. Status: "
                     << status;
          controller->NotifyUpdateDone();
          return;
        }

        // Fetch publishers via callback
        auto onPublishers = base::BindOnce(
            [](FeedController* controller, const std::string& body,
               const std::string& etag, Publishers publishers) {
              // Handle no publishers
              if (publishers.empty()) {
                LOG(ERROR) << "Brave News Publisher list was empty";
                controller->NotifyUpdateDone();
                return;
              }
              // Get history hosts via callback
              auto onHistory = base::BindOnce(
                  [](FeedController* controller, const std::string& body,
                     const std::string& etag, Publishers publishers,
                     history::QueryResults results) {
                    std::unordered_set<std::string> history_hosts;
                    for (const auto& item : results) {
                      auto host = item.url().host();
                      history_hosts.insert(host);
                    }
                    VLOG(1) << "history hosts # " << history_hosts.size();
                    // Parse directly to in-memory property
                    controller->ResetFeed();
                    if (BuildFeed(body, history_hosts, &publishers,
                                  &controller->current_feed_)) {
                      // Only mark cache time of remote request if
                      // parsing was successful
                      controller->current_feed_etag_ = etag;
                    } else {
                      VLOG(1) << "ParseFeed reported failure.";
                    }
                    // Let any callbacks know that the data is ready or errored.
                    controller->NotifyUpdateDone();
                  },
                  base::Unretained(controller), std::move(body),
                  std::move(etag), std::move(publishers));
              history::QueryOptions options;
              options.max_count = 2000;
              options.SetRecentDayRange(14);
              controller->history_service_->QueryHistory(
                  std::u16string(), options, std::move(onHistory),
                  &controller->task_tracker_);
            },
            base::Unretained(controller), std::move(body), std::move(etag));
        controller->publishers_controller_->GetOrFetchPublishers(
            std::move(onPublishers));
      },
      base::Unretained(this));
  GURL feed_url(GetFeedUrl());
  VLOG(1) << "Making feed request to " << feed_url.spec();
  api_request_helper_->Request("GET", feed_url, "", "", true,
                               std::move(onRequest),
                               brave::private_cdn_headers);
}

void FeedController::EnsureFeedIsCached() {
  VLOG(1) << "EnsureFeedIsCached";
  GetOrFetchFeed(
      base::BindOnce([]() { VLOG(1) << "EnsureFeedIsCached callback"; }));
}

void FeedController::UpdateIfRemoteChanged() {
  // If already updating, nothing to do,
  // we don't want to collide with an update
  // which starts and completes before our HEAD
  // request completes (which admittedly is very unlikely).
  if (is_update_in_progress_) {
    return;
  }
  // Get new Etag
  api_request_helper_->Request(
      "HEAD", GetFeedUrl(), "", "", true,
      base::BindOnce(
          [](FeedController* controller, const int status,
             const std::string& body,
             const base::flat_map<std::string, std::string>& headers) {
            std::string etag;
            if (headers.contains(kEtagHeaderKey)) {
              etag = headers.at(kEtagHeaderKey);
            }
            // Empty etag means perhaps server isn't supporting
            // the header right now, so we assume we should
            // always fetch the body at these times.
            if (etag.empty()) {
              LOG(ERROR) << "Brave News did not get correct etag, "
                            "therefore assuming etags aren't working and feed "
                            "changed.";
              controller->EnsureFeedIsUpdating();
              return;
            }
            VLOG(1) << "Comparing feed etag - "
                       "Original: "
                    << controller->current_feed_etag_ << " Remote: " << etag;
            // Compare remote etag with last feed fetch.
            if (controller->current_feed_etag_ == etag) {
              // Nothing to do
              return;
            }
            // Needs update
            controller->EnsureFeedIsUpdating();
          },
          base::Unretained(this)),
      brave::private_cdn_headers);
}

void FeedController::ClearCache() {
  ResetFeed();
}

void FeedController::OnPublishersUpdated(PublishersController* controller) {
  VLOG(1) << "OnPublishersUpdated";
  EnsureFeedIsUpdating();
}

void FeedController::GetOrFetchFeed(base::OnceClosure callback) {
  VLOG(1) << "getorfetch feed(oc) start: "
          << on_current_update_complete_->is_signaled();
  // If in-memory feed is, no need to wait, otherwise wait for fetch
  // to be complete.
  if (!current_feed_.hash.empty()) {
    VLOG(1) << "getorfetchfeed(oc) from cache";
    std::move(callback).Run();
    return;
  }
  // Ensure feed is currently being fetched.
  // Subscribe to result of current feed fetch.
  on_current_update_complete_->Post(FROM_HERE, std::move(callback));
  EnsureFeedIsUpdating();
}

void FeedController::ResetFeed() {
  current_feed_.featured_item = nullptr;
  current_feed_.hash = "";
  current_feed_.pages.clear();
}

void FeedController::NotifyUpdateDone() {
  // Let any callbacks know that the data is ready.
  on_current_update_complete_->Signal();
  // Reset the OneShotEvent so that future requests
  // can be waited for.
  is_update_in_progress_ = false;
  on_current_update_complete_ = std::make_unique<base::OneShotEvent>();
}

}  // namespace brave_news
