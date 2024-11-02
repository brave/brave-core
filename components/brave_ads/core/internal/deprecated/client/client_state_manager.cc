/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

namespace {
constexpr size_t kMaximumPurchaseIntentSignalHistoryEntriesPerSegment = 100;
}  // namespace

ClientStateManager::ClientStateManager() = default;

ClientStateManager::~ClientStateManager() = default;

// static
ClientStateManager& ClientStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetClientStateManager();
}

void ClientStateManager::LoadState(InitializeCallback callback) {
  BLOG(3, "Loading client state");

  GetAdsClient().Load(
      kClientJsonFilename,
      base::BindOnce(&ClientStateManager::LoadCallback,
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

  SaveState();
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

  SaveState();
}

const TextClassificationProbabilityList&
ClientStateManager::GetTextClassificationProbabilitiesHistory() const {
  CHECK(is_initialized_);

  return client_.text_classification_probabilities;
}

///////////////////////////////////////////////////////////////////////////////

void ClientStateManager::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving client state");

  GetAdsClient().Save(kClientJsonFilename, client_.ToJson(),
                      base::BindOnce([](const bool success) {
                        if (!success) {
                          return BLOG(0, "Failed to save client state");
                        }

                        BLOG(9, "Successfully saved client state");
                      }));
}

void ClientStateManager::LoadCallback(InitializeCallback callback,
                                      const std::optional<std::string>& json) {
  if (!json) {
    BLOG(3, "Client state does not exist, creating default state");

    is_initialized_ = true;
    client_ = {};

    SaveState();
  } else {
    if (!FromJson(*json)) {
      BLOG(1, "Failed to parse client state: " << *json);

      return std::move(callback).Run(/*success=*/false);
    }

    BLOG(3, "Successfully loaded client state");

    is_initialized_ = true;
  }

  std::move(callback).Run(/*success=*/true);
}

bool ClientStateManager::FromJson(const std::string& json) {
  ClientInfo client;
  if (!client.FromJson(json)) {
    return false;
  }

  client_ = client;

  return true;
}

}  // namespace brave_ads
