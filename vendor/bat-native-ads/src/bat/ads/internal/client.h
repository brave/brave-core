/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_H_
#define BAT_ADS_INTERNAL_CLIENT_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <memory>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client_state.h"

namespace ads {

class AdsImpl;

class Client {
 public:
  Client(AdsImpl* ads, AdsClient* ads_client);
  ~Client();

  void Initialize(InitializeCallback callback);

  void AppendAdHistoryToAdsShownHistory(const AdHistory& ad_history);
  const std::deque<AdHistory> GetAdsShownHistory() const;
  AdContent::LikeAction ToggleAdThumbUp(const std::string& id,
                                        const std::string& creative_set_id,
                                        AdContent::LikeAction action);
  AdContent::LikeAction ToggleAdThumbDown(const std::string& id,
                                          const std::string& creative_set_id,
                                          AdContent::LikeAction action);
  CategoryContent::OptAction ToggleAdOptInAction(
      const std::string& category,
      CategoryContent::OptAction action);
  CategoryContent::OptAction ToggleAdOptOutAction(
      const std::string& category,
      CategoryContent::OptAction action);
  bool ToggleSaveAd(const std::string& id,
                    const std::string& creative_set_id,
                    bool saved);
  bool ToggleFlagAd(const std::string& id,
                    const std::string& creative_set_id,
                    bool flagged);
  bool IsFilteredCategory(const std::string& category) const;
  bool IsFilteredAd(const std::string& creative_set_id) const;
  bool IsFlaggedAd(const std::string& creative_set_id) const;
  void UpdateAdUUID();
  void UpdateAdsUUIDSeen(const std::string& uuid, uint64_t value);
  const std::map<std::string, uint64_t> GetAdsUUIDSeen();
  void ResetAdsUUIDSeen(const CreativeAdNotifications& ads);
  void UpdatePublisherAdsUUIDSeen(const std::string& uuid, uint64_t value);
  const std::map<std::string, uint64_t> GetPublisherAdsUUIDSeen();
  void ResetPublisherAdsUUIDSeen(const CreativePublisherAds& ads);
  void SetNextCheckServeAdNotificationTimestampInSeconds(
      const uint64_t timestamp_in_seconds);
  uint64_t GetNextCheckServeAdNotificationTimestampInSeconds();
  void SetAvailable(const bool available);
  bool GetAvailable() const;
  void FlagShoppingState(const std::string& url, const uint64_t score);
  void UnflagShoppingState();
  bool GetShoppingState();
  void FlagSearchState(const std::string& url, const uint64_t score);
  void UnflagSearchState(const std::string& url);
  bool GetSearchState();
  void UpdateLastUserActivity();
  uint64_t GetLastUserActivity();
  void UpdateLastUserIdleStopTime();
  void SetUserModelLanguage(const std::string& language);
  const std::string GetUserModelLanguage();
  void SetUserModelLanguages(
      const std::vector<std::string>& languages);
  const std::vector<std::string> GetUserModelLanguages();
  void SetLastPageClassification(const std::string& classification);
  const std::string GetLastPageClassification();
  void AppendPageScoreToPageScoreHistory(
      const std::vector<double>& page_score);
  const std::deque<std::vector<double>> GetPageScoreHistory();
  void AppendTimestampToCreativeSetHistoryForUuid(
      const std::string& uuid,
      const uint64_t timestamp_in_seconds);
  const std::map<std::string, std::deque<uint64_t>>
      GetCreativeSetHistory() const;
  void AppendTimestampToAdConversionHistoryForUuid(
      const std::string& creative_set_id,
      const uint64_t timestamp_in_seconds);
  const std::map<std::string, std::deque<uint64_t>>
      GetAdConversionHistory() const;
  void AppendTimestampToCampaignHistoryForUuid(
      const std::string& uuid,
      const uint64_t timestamp_in_seconds);
  const std::map<std::string, std::deque<uint64_t>>
      GetCampaignHistory() const;
  std::string GetVersionCode() const;
  void SetVersionCode(const std::string& value);

  void RemoveAllHistory();

 private:
  bool is_initialized_;

  InitializeCallback callback_;

  void SaveState();
  void OnStateSaved(const Result result);

  void LoadState();
  void OnStateLoaded(const Result result, const std::string& json);

  bool FromJson(const std::string& json);

  AdsImpl* ads_;  // NOT OWNED
  AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<ClientState> client_state_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_H_
