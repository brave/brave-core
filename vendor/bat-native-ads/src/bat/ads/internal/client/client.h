/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_CLIENT_H_
#define BAT_ADS_INTERNAL_CLIENT_CLIENT_H_

#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <string>

#include "bat/ads/ad_content.h"
#include "bat/ads/ad_history.h"
#include "bat/ads/ads.h"
#include "bat/ads/category_content.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/classification/page_classifier/page_classifier.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_history.h"
#include "bat/ads/internal/client/client_state.h"
#include "bat/ads/internal/client/preferences/filtered_ad.h"
#include "bat/ads/internal/client/preferences/filtered_category.h"
#include "bat/ads/internal/client/preferences/flagged_ad.h"
#include "bat/ads/internal/client/preferences/saved_ad.h"
#include "bat/ads/result.h"

namespace ads {

class AdsImpl;

class Client {
 public:
  explicit Client(
      AdsImpl* ads);

  ~Client();

  void Initialize(
      InitializeCallback callback);

  FilteredAdsList get_filtered_ads() const;
  FilteredCategoriesList get_filtered_categories() const;
  FlaggedAdsList get_flagged_ads() const;

  void AppendAdHistoryToAdsHistory(
      const AdHistory& ad_history);
  const std::deque<AdHistory>& GetAdsHistory() const;
  void AppendToPurchaseIntentSignalHistoryForSegment(
      const std::string& segment,
      const PurchaseIntentSignalHistory& history);
  const PurchaseIntentSignalSegmentHistoryMap&
      GetPurchaseIntentSignalHistory() const;
  AdContent::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContent::LikeAction action);
  AdContent::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContent::LikeAction action);
  CategoryContent::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContent::OptAction action);
  CategoryContent::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContent::OptAction action);
  bool ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved);
  bool ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged);
  void UpdateAdUUID();

  void UpdateSeenAdNotification(
      const std::string& creative_instance_id,
      const uint64_t value);
  const std::map<std::string, uint64_t>& GetSeenAdNotifications();
  void ResetSeenAdNotifications(
      const CreativeAdNotificationList& ads);

  void UpdateSeenAdvertiser(
      const std::string& advertiser_id,
      const uint64_t value);
  const std::map<std::string, uint64_t>& GetSeenAdvertisers();
  void ResetSeenAdvertisers(
      const CreativeAdNotificationList& ads);

  void SetNextCheckServeAdNotificationDate(
      const base::Time& next_check_serve_ad_date);
  base::Time GetNextCheckServeAdNotificationDate();

  void SetAvailable(
      const bool available);
  bool GetAvailable() const;

  void AppendPageProbabilitiesToHistory(
      const classification::PageProbabilitiesMap& page_probabilities);
  const classification::PageProbabilitiesList& GetPageProbabilitiesHistory();
  void AppendTimestampToCreativeSetHistory(
      const std::string& creative_instance_id,
      const uint64_t timestamp_in_seconds);
  const std::map<std::string, std::deque<uint64_t>>&
      GetCreativeSetHistory() const;
  void AppendTimestampToAdConversionHistory(
      const std::string& creative_set_id,
      const uint64_t timestamp_in_seconds);
  const std::map<std::string, std::deque<uint64_t>>&
      GetAdConversionHistory() const;
  void AppendTimestampToCampaignHistory(
      const std::string& creative_instance_id,
      const uint64_t timestamp_in_seconds);
  const std::map<std::string, std::deque<uint64_t>>&
      GetCampaignHistory() const;
  std::string GetVersionCode() const;
  void SetVersionCode(
      const std::string& value);

  void RemoveAllHistory();

 private:
  bool is_initialized_;

  InitializeCallback callback_;

  void Save();
  void OnSaved(const Result result);

  void Load();
  void OnLoaded(const Result result, const std::string& json);

  bool FromJson(const std::string& json);

  AdsImpl* ads_;  // NOT OWNED

  std::unique_ptr<ClientState> client_state_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_CLIENT_H_
