/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_STATE_MANAGER_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_alias.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class ClientStateManager final {
 public:
  ClientStateManager();

  ClientStateManager(const ClientStateManager&) = delete;
  ClientStateManager& operator=(const ClientStateManager&) = delete;

  ClientStateManager(ClientStateManager&&) noexcept = delete;
  ClientStateManager& operator=(ClientStateManager&&) noexcept = delete;

  ~ClientStateManager();

  static ClientStateManager& GetInstance();

  void LoadState(InitializeCallback callback);

  void AppendToPurchaseIntentSignalHistoryForSegment(
      const std::string& segment,
      const PurchaseIntentSignalHistoryInfo& history);
  const PurchaseIntentSignalHistoryMap& GetPurchaseIntentSignalHistory() const;

  void AppendTextClassificationProbabilitiesToHistory(
      const TextClassificationProbabilityMap& probabilities);
  const TextClassificationProbabilityList&
  GetTextClassificationProbabilitiesHistory() const;

 private:
  void SaveState();

  void LoadCallback(InitializeCallback callback,
                    const std::optional<std::string>& json);

  [[nodiscard]] bool FromJson(const std::string& json);

  ClientInfo client_;

  bool is_initialized_ = false;

  base::WeakPtrFactory<ClientStateManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_STATE_MANAGER_H_
