/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_database_table.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"

namespace brave_ads {

namespace {

constexpr size_t kMaximumPurchaseIntentSignalHistoryEntriesPerSegment = 100;

void LogPruneTextClassificationProbabilitiesToMaximumEntries(bool success) {
  if (!success) {
    return BLOG(0, "Failed to prune text classification probabilities");
  }

  BLOG(9, "Successfully pruned text classification probabilities");
}

}  // namespace

ClientStateManager::ClientStateManager() = default;

ClientStateManager::~ClientStateManager() = default;

// static
ClientStateManager& ClientStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetClientStateManager();
}

void ClientStateManager::LoadState(ResultCallback callback) {
  BLOG(3, "Loading client state");

  database::table::PurchaseIntentSignalHistory
      purchase_intent_signal_history_database_table;
  purchase_intent_signal_history_database_table.GetAll(base::BindOnce(
      &ClientStateManager::GetAllPurchaseIntentSignalHistoryCallback,
      weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ClientStateManager::AppendToPurchaseIntentSignalHistoryForSegment(
    const std::string& segment,
    const PurchaseIntentSignalHistoryInfo& history) {
  CHECK(is_initialized_);

  if (client_.purchase_intent_signal_history.find(segment) ==
      client_.purchase_intent_signal_history.cend()) {
    client_.purchase_intent_signal_history.insert({segment, {}});
  }

  client_.purchase_intent_signal_history.at(segment).push_back(history);

  if (client_.purchase_intent_signal_history.at(segment).size() >
      kMaximumPurchaseIntentSignalHistoryEntriesPerSegment) {
    client_.purchase_intent_signal_history.at(segment).pop_back();
  }

  database::table::PurchaseIntentSignalHistory
      purchase_intent_signal_history_database_table;
  purchase_intent_signal_history_database_table.SaveForSegment(
      segment, client_.purchase_intent_signal_history.at(segment),
      base::BindOnce([](bool success) {
        if (!success) {
          return BLOG(0, "Failed to save purchase intent signal history");
        }

        BLOG(9, "Successfully saved purchase intent signal history");
      }));
}

const PurchaseIntentSignalHistoryMap&
ClientStateManager::GetPurchaseIntentSignalHistory() const {
  CHECK(is_initialized_);

  return client_.purchase_intent_signal_history;
}

void ClientStateManager::AppendTextClassificationProbabilitiesToHistory(
    const TextClassificationProbabilityMap& probabilities) {
  CHECK(is_initialized_);

  client_.text_classification_probabilities.push_front(probabilities);

  const size_t maximum_entries =
      kTextClassificationPageProbabilitiesHistorySize.Get();
  if (client_.text_classification_probabilities.size() > maximum_entries) {
    client_.text_classification_probabilities.resize(maximum_entries);
  }

  database::table::TextClassificationProbabilities
      text_classification_probabilities_database_table;
  text_classification_probabilities_database_table.Save(
      probabilities, base::Time::Now(),
      base::BindOnce(&ClientStateManager::
                         SaveTextClassificationProbabilitiesToHistoryCallback,
                     weak_factory_.GetWeakPtr(), maximum_entries));
}

const TextClassificationProbabilityList&
ClientStateManager::GetTextClassificationProbabilitiesHistory() const {
  CHECK(is_initialized_);

  return client_.text_classification_probabilities;
}

///////////////////////////////////////////////////////////////////////////////

void ClientStateManager::GetAllPurchaseIntentSignalHistoryCallback(
    ResultCallback callback,
    bool success,
    const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history) {
  if (!success) {
    BLOG(0, "Failed to load purchase intent signal history");
    return std::move(callback).Run(/*success=*/false);
  }

  client_.purchase_intent_signal_history = purchase_intent_signal_history;

  database::table::TextClassificationProbabilities
      text_classification_probabilities_database_table;
  text_classification_probabilities_database_table.GetAll(base::BindOnce(
      &ClientStateManager::GetAllTextClassificationProbabilitiesCallback,
      weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ClientStateManager::GetAllTextClassificationProbabilitiesCallback(
    ResultCallback callback,
    bool success,
    const TextClassificationProbabilityList&
        text_classification_probabilities) {
  if (!success) {
    BLOG(0, "Failed to load text classification probabilities");
    return std::move(callback).Run(/*success=*/false);
  }

  client_.text_classification_probabilities = text_classification_probabilities;

  BLOG(3, "Successfully loaded client state");
  is_initialized_ = true;
  std::move(callback).Run(/*success=*/true);
}

void ClientStateManager::SaveTextClassificationProbabilitiesToHistoryCallback(
    size_t maximum_entries,
    bool success) {
  if (!success) {
    return BLOG(0, "Failed to save text classification probabilities");
  }

  BLOG(9, "Successfully saved text classification probabilities");

  database::table::TextClassificationProbabilities
      text_classification_probabilities_database_table;
  text_classification_probabilities_database_table.PruneToMaximumEntries(
      maximum_entries,
      base::BindOnce(&LogPruneTextClassificationProbabilitiesToMaximumEntries));
}

}  // namespace brave_ads
