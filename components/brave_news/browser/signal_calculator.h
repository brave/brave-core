// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNAL_CALCULATOR_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNAL_CALCULATOR_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

using Signal = mojom::SignalPtr;
using Signals =
    base::flat_map</*channel name or publisher_id*/ std::string, Signal>;
using SignalsCallback = base::OnceCallback<void(Signals)>;

class SubscriptionsSnapshot;

class SignalCalculator {
 public:
  SignalCalculator(PublishersController& publishers_controller,
                   ChannelsController& channels_controller,
                   BackgroundHistoryQuerier& history_querier);
  SignalCalculator(const SignalCalculator&) = delete;
  SignalCalculator& operator=(const SignalCalculator&) = delete;
  ~SignalCalculator();

  void GetSignals(const SubscriptionsSnapshot& subscriptions,
                  const FeedItems& feed,
                  SignalsCallback callback);

 private:
  void OnGotHistory(const SubscriptionsSnapshot& subscriptions,
                    std::vector<mojom::FeedItemMetadataPtr> articles,
                    SignalsCallback callback,
                    history::QueryResults results);

  double GetSubscribedWeight(const mojom::PublisherPtr& publisher);

  base::CancelableTaskTracker task_tracker_;

  raw_ref<PublishersController> publishers_controller_;
  raw_ref<ChannelsController> channels_controller_;
  raw_ref<BackgroundHistoryQuerier> history_querier_;

  base::WeakPtrFactory<SignalCalculator> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNAL_CALCULATOR_H_
