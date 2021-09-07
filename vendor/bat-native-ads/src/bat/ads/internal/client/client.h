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

#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_aliases.h"
#include "bat/ads/internal/ad_targeting/data_types/contextual/text_classification/text_classification_aliases.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/client/preferences/filtered_ad_info.h"
#include "bat/ads/internal/client/preferences/filtered_category_info.h"
#include "bat/ads/internal/client/preferences/flagged_ad_info.h"
#include "bat/ads/internal/client/preferences/saved_ad_info.h"

namespace ads {

namespace ad_targeting {
struct PurchaseIntentSignalHistoryInfo;
}  // namespace ad_targeting

struct AdContentInfo;
struct AdHistoryInfo;
struct AdInfo;
struct CategoryContentInfo;
struct ClientInfo;

class Client {
 public:
  Client();

  ~Client();

  static Client* Get();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  FilteredAdList get_filtered_ads() const;
  FilteredCategoryList get_filtered_categories() const;
  FlaggedAdList get_flagged_ads() const;

  void AppendAdHistory(const AdHistoryInfo& ad_history);
  const std::deque<AdHistoryInfo>& GetAdsHistory() const;

  void AppendToPurchaseIntentSignalHistoryForSegment(
      const std::string& segment,
      const ad_targeting::PurchaseIntentSignalHistoryInfo& history);
  const ad_targeting::PurchaseIntentSignalHistoryMap&
  GetPurchaseIntentSignalHistory() const;

  AdContentInfo::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction action);
  AdContentInfo::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction action);
  AdContentInfo::LikeAction GetLikeActionForSegment(const std::string& segment);

  CategoryContentInfo::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContentInfo::OptAction action);
  CategoryContentInfo::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContentInfo::OptAction action);
  CategoryContentInfo::OptAction GetOptActionForSegment(
      const std::string& segment);

  bool ToggleSaveAd(const std::string& creative_instance_id,
                    const std::string& creative_set_id,
                    const bool saved);
  bool GetSavedAdForCreativeInstanceId(const std::string& creative_instance_id);

  bool ToggleFlagAd(const std::string& creative_instance_id,
                    const std::string& creative_set_id,
                    const bool flagged);
  bool GetFlaggedAdForCreativeInstanceId(
      const std::string& creative_instance_id);

  void UpdateSeenAd(const AdInfo& ad);
  const std::map<std::string, bool>& GetSeenAdsForType(const AdType& type);
  void ResetSeenAdsForType(const CreativeAdList& ads, const AdType& type);
  void ResetAllSeenAdsForType(const AdType& type);
  const std::map<std::string, bool>& GetSeenAdvertisersForType(
      const AdType& type);
  void ResetSeenAdvertisersForType(const CreativeAdList& ads,
                                   const AdType& type);
  void ResetAllSeenAdvertisersForType(const AdType& type);

  void SetNextAdServingInterval(const base::Time& next_check_serve_ad_date);
  base::Time GetNextAdServingInterval();

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
