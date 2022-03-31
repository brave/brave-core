// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/publishers_controller.h"

#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/publishers_parsing.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/pref_names.h"

namespace brave_news {

PublishersController::PublishersController(
    PrefService* prefs,
    api_request_helper::APIRequestHelper* api_request_helper)
    : prefs_(prefs),
      api_request_helper_(api_request_helper),
      on_current_update_complete_(new base::OneShotEvent()) {}

PublishersController::~PublishersController() = default;

void PublishersController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PublishersController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// To be consumed outside of the class - provides a clone
void PublishersController::GetOrFetchPublishers(
    GetPublishersCallback callback,
    bool wait_for_current_update /* = false */) {
  GetOrFetchPublishers(
      base::BindOnce(
          [](PublishersController* controller, GetPublishersCallback callback) {
            // Either there was already data, or the fetch was complete
            // (with success or error, so we would still check for valid data
            // again, but it's fine to just send the empty array). Provide data
            // clone for ownership outside of this class.
            Publishers clone;
            for (auto const& kv : controller->publishers_) {
              clone.insert_or_assign(kv.first, kv.second->Clone());
            }
            std::move(callback).Run(std::move(clone));
          },
          base::Unretained(this), std::move(callback)),
      wait_for_current_update);
}

// To be consumed internally - provides no data so that we don't need to clone,
// as data can be accessed via class property
void PublishersController::GetOrFetchPublishers(base::OnceClosure callback,
                                                bool wait_for_current_update) {
  // If in-memory data is already present, no need to wait,
  // otherwise wait for fetch to be complete.
  // Also don't wait if there's an update in progress and this caller
  // wishes to wait.
  if (!publishers_.empty() &&
      (!wait_for_current_update || !is_update_in_progress_)) {
    std::move(callback).Run();
    return;
  }
  // Ensure data is currently being fetched and subscribe to know
  // when that is complete.
  on_current_update_complete_->Post(FROM_HERE, std::move(callback));
  EnsurePublishersIsUpdating();
}

void PublishersController::EnsurePublishersIsUpdating() {
  // Only 1 update at a time, other calls for data will wait for
  // the current operation via the `on_current_update_complete_` OneShotEvent.
  if (is_update_in_progress_) {
    return;
  }
  is_update_in_progress_ = true;
  GURL sources_url("https://" + brave_today::GetHostname() + "/sources." +
                   brave_today::GetRegionUrlPart() + "json");
  auto onRequest = base::BindOnce(
      [](PublishersController* controller, const int status,
         const std::string& body,
         const base::flat_map<std::string, std::string>& headers) {
        VLOG(1) << "Downloaded sources, status: " << status;
        // TODO(petemill): handle bad status or response
        Publishers publisher_list;
        ParseCombinedPublisherList(body, &publisher_list);
        // Add user enabled statuses
        const base::Value* publisher_prefs =
            controller->prefs_->GetDictionary(prefs::kBraveTodaySources);
        for (auto kv : publisher_prefs->DictItems()) {
          auto publisher_id = kv.first;
          auto is_user_enabled = kv.second.GetIfBool();
          if (publisher_list.contains(publisher_id) &&
              is_user_enabled.has_value()) {
            publisher_list[publisher_id]->user_enabled_status =
                (is_user_enabled.value()
                     ? brave_news::mojom::UserEnabled::ENABLED
                     : brave_news::mojom::UserEnabled::DISABLED);
          } else {
            VLOG(1) << "Publisher list did not contain publisher found in"
                       "user prefs: "
                    << publisher_id;
          }
        }
        // Add direct feeds
        std::vector<mojom::PublisherPtr> direct_publishers;
        ParseDirectPublisherList(
            controller->prefs_->GetDictionary(prefs::kBraveTodayDirectFeeds),
            &direct_publishers);
        for (auto it = direct_publishers.begin(); it != direct_publishers.end();
             it++) {
          auto move_it = std::make_move_iterator(it);
          auto publisher = *move_it;
          publisher_list.insert_or_assign(publisher->publisher_id,
                                          std::move(publisher));
        }

        // Set memory cache
        controller->publishers_ = std::move(publisher_list);
        // Let any callback know that the data is ready.
        VLOG(1) << "Notify subscribers to publishers data";
        // One-shot subscribers
        controller->on_current_update_complete_->Signal();
        controller->is_update_in_progress_ = false;
        controller->on_current_update_complete_ =
            std::make_unique<base::OneShotEvent>();
        // Observers
        for (auto& observer : controller->observers_) {
          observer.OnPublishersUpdated(controller);
        }
      },
      base::Unretained(this));
  api_request_helper_->Request("GET", sources_url, "", "", true,
                               std::move(onRequest),
                               brave::private_cdn_headers);
}

void PublishersController::ClearCache() {
  publishers_.clear();
}

}  // namespace brave_news
