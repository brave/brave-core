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
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_advertiser_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_category_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/flagged_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_alias.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

struct AdInfo;

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

  const FilteredAdvertiserList& GetFilteredAdvertisers() const;
  const FilteredCategoryList& GetFilteredCategories() const;
  const FlaggedAdList& GetFlaggedAds() const;

  void AppendAdHistoryItem(const AdHistoryItemInfo& ad_history_item);
  const AdHistoryList& GetAdHistory() const;

  void AppendToPurchaseIntentSignalHistoryForSegment(
      const std::string& segment,
      const PurchaseIntentSignalHistoryInfo& history);
  const PurchaseIntentSignalHistoryMap& GetPurchaseIntentSignalHistory() const;

  mojom::ReactionType ToggleLikeAd(const AdHistoryItemInfo& ad_history_item);
  mojom::ReactionType ToggleDislikeAd(const AdHistoryItemInfo& ad_history_item);
  mojom::ReactionType GetReactionTypeForAd(const AdInfo& ad);

  mojom::ReactionType ToggleLikeSegment(
      const AdHistoryItemInfo& ad_history_item);
  mojom::ReactionType ToggleDislikeSegment(
      const AdHistoryItemInfo& ad_history_item);
  mojom::ReactionType GetReactionTypeForSegment(const std::string& segment);

  bool ToggleSaveAd(const AdHistoryItemInfo& ad_history_item);

  bool ToggleMarkAdAsInappropriate(const AdHistoryItemInfo& ad_history_item);

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
