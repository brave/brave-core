// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/new_feed_controller.h"

#include <iterator>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/combined_feed_parsing.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/feed_building.h"
#include "brave/components/brave_news/browser/feed_controller.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "components/history/core/browser/history_types.h"

namespace brave_news {

NewFeedController::NewFeedController(
    PublishersController* publishers_controller,
    ChannelsController* channels_controller,
    history::HistoryService* history_service,
    api_request_helper::APIRequestHelper* api_request_helper,
    PrefService* prefs)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      history_service_(history_service),
      api_request_helper_(api_request_helper),
      prefs_(prefs),
      on_current_update_complete_(new base::OneShotEvent()),
      publishers_observation_(this) {
  publishers_observation_.Observe(publishers_controller);
}

NewFeedController::~NewFeedController() = default;

void NewFeedController::EnsureFeedIsUpdating() {
  if (is_update_in_progress_) {
    return;
  }

  is_update_in_progress_ = true;

  publishers_controller_->GetOrFetchPublishers(base::BindOnce(
      [](NewFeedController* controller, Publishers publishers) {
        auto feed_items_handler = base::BindOnce(
            [](NewFeedController* controller, Publishers publishers,
               FeedItems feed_items) {
              auto on_history = base::BindOnce(
                  [](FeedController* controller, FeedItems feed_items,
                     Publishers publishers, history::QueryResults results) {
                    std::unordered_set<std::string> history_hosts;
                    for (const auto& item : results) {
                      auto host = item.url().host();
                      history_hosts.insert(host);
                    }
                  });
            },
            base::Unretained(controller), std::move(publishers));

        controller->FetchCombinedFeed(publishers,
                                      std::move(feed_items_handler));
      },
      base::Unretained(this)));
}

void NewFeedController::FetchCombinedFeed(Publishers publishers,
                                          GetFeedItemsCallback callback) {
  auto locales = GetMinimalLocalesSet(channels_controller_->GetChannelLocales(),
                                      publishers);
  auto locales_fetched_callback = base::BarrierCallback<FeedItems>(
      locales.size(), base::BindOnce(
                          [](GetFeedItemsCallback callback,
                             std::vector<FeedItems> feed_items_unflat) {
                            std::size_t total_size = 0;
                            for (const auto& collection : feed_items_unflat) {
                              total_size += collection.size();
                            }

                            FeedItems all_feed_items;
                            all_feed_items.reserve(total_size);

                            for (auto& collection : feed_items_unflat) {
                              all_feed_items.insert(
                                  all_feed_items.end(),
                                  std::make_move_iterator(collection.begin()),
                                  std::make_move_iterator(collection.end()));
                            }

                            std::move(callback).Run(std::move(all_feed_items));
                          },
                          std::move(callback)));

  for (const auto& locale : locales) {
    auto response_handler = base::BindOnce(
        [](NewFeedController* controller, std::string locale,
           GetFeedItemsCallback callback,
           api_request_helper::APIRequestResult api_request_result) {
          if (api_request_result.response_code() != 200 ||
              api_request_result.value_body().is_none()) {
            std::move(callback).Run({});
            return;
          }

          std::move(callback).Run(
              ParseFeedItems(api_request_result.value_body()));
        },
        base::Unretained(this), locale, locales_fetched_callback);
  }
}

void NewFeedController::NotifyUpdateDone() {
  on_current_update_complete_->Signal();

  is_update_in_progress_ = false;
  on_current_update_complete_ = std::make_unique<base::OneShotEvent>();
}

}  // namespace brave_news
