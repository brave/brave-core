/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <memory>

#include "ads_impl.h"
#include "bat/ads/ads_client.h"
#include "client_state.h"

namespace ads {

class AdsImpl;

class Client {
 public:
  Client(AdsImpl* ads, AdsClient* ads_client);
  ~Client();

  void SaveState();
  void LoadState();

  void AppendCurrentTimeToAdsShownHistory();
  const std::deque<uint64_t> GetAdsShownHistory();
  void GetAdsShownHistory(const std::deque<uint64_t>& history);
  void UpdateAdUUID();
  void UpdateAdsUUIDSeen(const std::string& uuid, uint64_t value);
  const std::map<std::string, uint64_t> GetAdsUUIDSeen();
  void ResetAdsUUIDSeen(const std::vector<AdInfo>& ads);
  void SetAvailable(const bool available);
  bool GetAvailable() const;
  void SetCurrentSSID(const std::string& ssid);
  void FlagShoppingState(const std::string& url, const uint64_t score);
  void UnflagShoppingState();
  bool GetShoppingState();
  void FlagSearchState(const std::string& url, const uint64_t score);
  void UnflagSearchState(const std::string& url);
  bool GetSearchState();
  void UpdateLastUserActivity();
  uint64_t GetLastUserActivity();
  void UpdateLastUserIdleStopTime();
  void SetLocale(const std::string& locale);
  const std::string GetLocale();
  void SetLocales(const std::vector<std::string>& locales);
  const std::vector<std::string> GetLocales();
  void AppendPageScoreToPageScoreHistory(
      const std::vector<double>& page_scores);
  const std::deque<std::vector<double>> GetPageScoreHistory();
  void AppendCurrentTimeToCreativeSetHistory(
      const std::string& creative_set_id);
  const std::map<std::string, std::deque<uint64_t>>
      GetCreativeSetHistory() const;
  void AppendCurrentTimeToCampaignHistory(
      const std::string& campaign_id);
  const std::map<std::string, std::deque<uint64_t>>
      GetCampaignHistory() const;
  const std::string GetCurrentPlace();

  void RemoveAllHistory();

 private:
  void OnStateSaved(const Result result);
  void OnStateLoaded(const Result result, const std::string& json);

  bool FromJson(const std::string& json);

  AdsImpl* ads_;  // NOT OWNED
  AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<ClientState> client_state_;
};

}  // namespace ads
