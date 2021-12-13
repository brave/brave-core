/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CLIENT_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CLIENT_CLIENT_H_

#include <deque>
#include <map>
#include <memory>
#include <string>

#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/ads_aliases.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_aliases.h"
#include "bat/ads/internal/ad_targeting/data_types/contextual/text_classification/text_classification_aliases.h"
#include "bat/ads/internal/bundle/creative_ad_info_aliases.h"
#include "bat/ads/internal/client/preferences/filtered_advertiser_info_aliases.h"
#include "bat/ads/internal/client/preferences/filtered_category_info_aliases.h"
#include "bat/ads/internal/client/preferences/flagged_ad_info_aliases.h"
#include "bat/ads/internal/client/preferences/saved_ad_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

namespace ad_targeting {
struct PurchaseIntentSignalHistoryInfo;
}  // namespace ad_targeting

class AdType;
struct AdContentInfo;
struct AdHistoryInfo;
struct AdInfo;
struct ClientInfo;

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

  void AppendAdHistory(const AdHistoryInfo& ad_history);
  const std::deque<AdHistoryInfo>& GetAdsHistory() const;

  void AppendToPurchaseIntentSignalHistoryForSegment(
      const std::string& segment,
      const ad_targeting::PurchaseIntentSignalHistoryInfo& history);
  const ad_targeting::PurchaseIntentSignalHistoryMap&
  GetPurchaseIntentSignalHistory() const;

  AdContentLikeActionType ToggleAdThumbUp(const AdContentInfo& ad_content);
  AdContentLikeActionType ToggleAdThumbDown(const AdContentInfo& ad_content);
  AdContentLikeActionType GetAdContentLikeActionTypeForSegment(
      const std::string& segment);

  CategoryContentOptActionType ToggleAdOptIn(
      const std::string& category,
      const CategoryContentOptActionType action_type);
  CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      const CategoryContentOptActionType action_type);
  CategoryContentOptActionType GetCategoryContentOptActionTypeForSegment(
      const std::string& segment);

  bool ToggleSavedAd(const AdContentInfo& ad_content);
  bool GetSavedAdForCreativeInstanceId(const std::string& creative_instance_id);

  bool ToggleFlaggedAd(const AdContentInfo& ad_content);
  bool GetFlaggedAdForCreativeInstanceId(
      const std::string& creative_instance_id);

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

  void SetServeAdAt(const base::Time& time);
  base::Time GetServeAdAt();

  void AppendTextClassificationProbabilitiesToHistory(
      const ad_targeting::TextClassificationProbabilitiesMap& probabilities);
  const ad_targeting::TextClassificationProbabilitiesList&
  GetTextClassificationProbabilitiesHistory();

  std::string GetVersionCode() const;
  void SetVersionCode(const std::string& value);

  void RemoveAllHistory();

 private:
  bool is_initialized_ = false;

  InitializeCallback callback_;

  void Save();
  void OnSaved(const bool success);

  void Load();
  void OnLoaded(const bool success, const std::string& json);

  bool FromJson(const std::string& json);

  std::unique_ptr<ClientInfo> client_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CLIENT_CLIENT_H_
