// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SUGGESTIONS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SUGGESTIONS_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"

namespace brave_news {

using GetSuggestedPublisherIdsCallback =
    mojom::BraveNewsController::GetSuggestedPublisherIdsCallback;

class SubscriptionsSnapshot;

class SuggestionsController {
 public:
  struct PublisherSimilarity {
    std::string publisher_id;
    double score;
  };

  using PublisherSimilarities =
      base::flat_map<std::string, std::vector<PublisherSimilarity>>;

  explicit SuggestionsController(
      PublishersController* publishers_controller,
      api_request_helper::APIRequestHelper* api_request_helper,
      BackgroundHistoryQuerier& history_querier);
  SuggestionsController(const SuggestionsController&) = delete;
  SuggestionsController& operator=(const SuggestionsController&) = delete;
  ~SuggestionsController();

  void GetSuggestedPublisherIds(const SubscriptionsSnapshot& subscriptions,
                                GetSuggestedPublisherIdsCallback callback);
  void EnsureSimilarityMatrixIsUpdating(
      const SubscriptionsSnapshot& subscriptions);

 private:
  friend class BraveNewsSuggestionsControllerTest;
  void GetOrFetchSimilarityMatrix(const SubscriptionsSnapshot& subscriptions,
                                  base::OnceClosure callback);
  std::vector<std::string> GetSuggestedPublisherIdsWithHistory(
      const Publishers& publishers,
      const history::QueryResults& history);

  bool is_update_in_progress_ = false;
  // Task tracker for HistoryService callbacks.
  base::CancelableTaskTracker task_tracker_;

  raw_ptr<PublishersController> publishers_controller_;
  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  raw_ref<BackgroundHistoryQuerier> history_querier_;
  std::unique_ptr<base::OneShotEvent> on_current_update_complete_;

  std::string locale_;
  PublisherSimilarities similarities_;

  base::WeakPtrFactory<SuggestionsController> weak_ptr_factory_{this};
};
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SUGGESTIONS_CONTROLLER_H_
