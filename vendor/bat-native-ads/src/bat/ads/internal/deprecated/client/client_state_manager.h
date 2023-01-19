/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_STATE_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_STATE_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_alias.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/deprecated/client/preferences/filtered_advertiser_info.h"
#include "bat/ads/internal/deprecated/client/preferences/filtered_category_info.h"
#include "bat/ads/internal/deprecated/client/preferences/flagged_ad_info.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace ads {

class AdType;
struct AdContentInfo;
struct AdInfo;
struct ClientInfo;

class ClientStateManager final {
 public:
  ClientStateManager();

  ClientStateManager(const ClientStateManager& other) = delete;
  ClientStateManager& operator=(const ClientStateManager& other) = delete;

  ClientStateManager(ClientStateManager&& other) noexcept = delete;
  ClientStateManager& operator=(ClientStateManager&& other) noexcept = delete;

  ~ClientStateManager();

  static ClientStateManager* GetInstance();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  const FilteredAdvertiserList& GetFilteredAdvertisers() const;
  const FilteredCategoryList& GetFilteredCategories() const;
  const FlaggedAdList& GetFlaggedAds() const;

  void AppendHistory(const HistoryItemInfo& history_item);
  const HistoryItemList& GetHistory() const;

  void AppendToPurchaseIntentSignalHistoryForSegment(
      const std::string& segment,
      const targeting::PurchaseIntentSignalHistoryInfo& history);
  const targeting::PurchaseIntentSignalHistoryMap&
  GetPurchaseIntentSignalHistory() const;

  AdContentLikeActionType ToggleAdThumbUp(const AdContentInfo& ad_content);
  AdContentLikeActionType ToggleAdThumbDown(const AdContentInfo& ad_content);
  AdContentLikeActionType GetAdContentLikeActionTypeForAdvertiser(
      const std::string& advertiser_id);

  CategoryContentOptActionType ToggleAdOptIn(
      const std::string& category,
      CategoryContentOptActionType action_type);
  CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      CategoryContentOptActionType action_type);
  CategoryContentOptActionType GetCategoryContentOptActionTypeForSegment(
      const std::string& segment);

  bool ToggleSavedAd(const AdContentInfo& ad_content);

  bool ToggleFlaggedAd(const AdContentInfo& ad_content);

  void UpdateSeenAd(const AdInfo& ad);
  const std::map<std::string, bool>& GetSeenAdsForType(const AdType& type);
  void ResetSeenAdsForType(const CreativeAdList& creative_ads,
                           const AdType& type);
  void ResetAllSeenAdsForType(const AdType& type);
  const std::map<std::string, bool>& GetSeenAdvertisersForType(
      const AdType& type);
  void ResetSeenAdvertisersForType(const CreativeAdList& creative_ads,
                                   const AdType& type);
  void ResetAllSeenAdvertisersForType(const AdType& type);

  void AppendTextClassificationProbabilitiesToHistory(
      const targeting::TextClassificationProbabilityMap& probabilities);
  const targeting::TextClassificationProbabilityList&
  GetTextClassificationProbabilitiesHistory();

  void RemoveAllHistory();

  bool is_mutated() const { return is_mutated_; }

 private:
  void Save();

  void Load(InitializeCallback callback);
  void OnLoaded(InitializeCallback callback,
                bool success,
                const std::string& json);

  bool FromJson(const std::string& json);

  std::unique_ptr<ClientInfo> client_;

  bool is_mutated_ = false;

  bool is_initialized_ = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_STATE_MANAGER_H_
