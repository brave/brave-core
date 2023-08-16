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
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

using Signal = mojom::SignalPtr;
using Signals = base::flat_map<std::string, Signal>;
using SignalsCallback = base::OnceCallback<void(Signals)>;

class SignalCalculator {
 public:
  SignalCalculator(PublishersController& publishers_controller,
                   ChannelsController& channels_controller,
                   PrefService& prefs,
                   history::HistoryService& history_service);
  SignalCalculator(const SignalCalculator&) = delete;
  SignalCalculator& operator=(const SignalCalculator&) = delete;
  ~SignalCalculator();

  void GetSignals(const FeedItems& feed, SignalsCallback callback);

 private:
  void OnGotHistory(std::vector<mojom::FeedItemMetadataPtr> articles,
                    SignalsCallback callback,
                    history::QueryResults results);

  bool IsPublisherSubscribed(const mojom::PublisherPtr& publisher);

  base::CancelableTaskTracker task_tracker_;

  raw_ref<PublishersController> publishers_controller_;
  raw_ref<ChannelsController> channels_controller_;
  raw_ref<PrefService> prefs_;
  raw_ref<history::HistoryService> history_service_;

  base::WeakPtrFactory<SignalCalculator> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNAL_CALCULATOR_H_
