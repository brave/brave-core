// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_RAW_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_RAW_FEED_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "base/scoped_observation.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news {

using FeedItems = std::vector<mojom::FeedItemPtr>;
using GetRawFeedCallback = base::OnceCallback<void(FeedItems)>;

class RawFeedController : public PublishersController::Observer {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFeedUpdated(const FeedItems& feed_items) = 0;
  };

  explicit RawFeedController(
      PublishersController& publishers_controller,
      ChannelsController& channels_controller,
      api_request_helper::APIRequestHelper& api_request_helper);
  ~RawFeedController() override;
  RawFeedController(const RawFeedController&) = delete;
  RawFeedController& operator=(const RawFeedController&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void GetOrFetchFeed(GetRawFeedCallback callback);
  void EnsureFeedIsUpdating();
  void UpdateRemoteIfChanged();
  void ClearCache();

  // PublishersController::Observer:
  void OnPublishersUpdated(PublishersController* controller) override;

 private:
  void ResetFeed();
  void NotifyUpdateDone();

  const raw_ref<PublishersController> publishers_controller_;
  const raw_ref<ChannelsController> channels_controller_;
  const raw_ref<api_request_helper::APIRequestHelper> api_request_helper_;

  FeedItems current_feed_items_;

  std::unique_ptr<base::OneShotEvent> on_current_update_complete_ =
      std::make_unique<base::OneShotEvent>();
  base::flat_map<std::string, std::string> locale_feed_etags_;
  bool is_update_in_progress_ = false;

  base::ObserverList<Observer> observers_;
  base::ScopedObservation<PublishersController, RawFeedController>
      publishers_observation_{this};

  base::WeakPtrFactory<RawFeedController> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_RAW_FEED_CONTROLLER_H_
