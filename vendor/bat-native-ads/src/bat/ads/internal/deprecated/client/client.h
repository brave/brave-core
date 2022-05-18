/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_H_

#include <map>
#include <memory>
#include <string>

#include "base/containers/circular_deque.h"
#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/ads_aliases.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_info_aliases.h"
#include "bat/ads/internal/deprecated/client/preferences/filtered_advertiser_info_aliases.h"
#include "bat/ads/internal/deprecated/client/preferences/filtered_category_info_aliases.h"
#include "bat/ads/internal/deprecated/client/preferences/flagged_ad_info_aliases.h"
#include "bat/ads/internal/deprecated/client/preferences/saved_ad_info_aliases.h"
#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_aliases.h"
#include "bat/ads/internal/targeting/data_types/contextual/text_classification/text_classification_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

namespace targeting {
struct PurchaseIntentSignalHistoryInfo;
}  // namespace targeting

class AdType;
struct AdContentInfo;
struct AdInfo;
struct ClientInfo;
struct HistoryItemInfo;

class Client final {
 public:
  Client();
  ~Client();

  static Client* Get();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  FilteredAdvertiserList GetFilteredAdvertisers() const;
  FilteredCategoryList GetFilteredCategories() const;
  FlaggedAdList GetFlaggedAds() const;

  void AppendHistory(const HistoryItemInfo& history_item);
  const base::circular_deque<HistoryItemInfo>& GetHistory() const;

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
      const CategoryContentOptActionType action_type);
  CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      const CategoryContentOptActionType action_type);
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

  void SetServeAdAt(const base::Time time);
  base::Time GetServeAdAt();

  void AppendTextClassificationProbabilitiesToHistory(
      const targeting::TextClassificationProbabilitiesMap& probabilities);
  const targeting::TextClassificationProbabilitiesList&
  GetTextClassificationProbabilitiesHistory();

  std::string GetVersionCode() const;
  void SetVersionCode(const std::string& value);

  void RemoveAllHistory();

  bool is_mutated() const { return is_mutated_; }

 private:
  void Save();
  void OnSaved(const bool success);

  void Load();
  void OnLoaded(const bool success, const std::string& json);

  bool FromJson(const std::string& json);

  std::unique_ptr<ClientInfo> client_;

  bool is_mutated_ = false;

  bool is_initialized_ = false;

  InitializeCallback callback_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_H_
