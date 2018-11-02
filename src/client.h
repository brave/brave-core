/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <memory>
#include <ctime>

#include "bat/ads/ads_client.h"
#include "bat/ads/callback_handler.h"
#include "client_state.h"

namespace ads {
class AdsImpl;
}  // namespace ads

namespace ads {

class Client: public ads::CallbackHandler {
 public:
  Client(ads::AdsImpl* ads, ads::AdsClient* ads_client);
  ~Client();

  bool LoadJson(const std::string& json);  // Deserialize
  void SaveJson();  // Serialize

  void AppendCurrentTimeToAdsShownHistory();
  std::deque<std::time_t> GetAdsShownHistory();
  void GetAdsShownHistory(const std::deque<std::time_t>& history);
  void UpdateAdUUID();
  void UpdateAdsUUIDSeen(const std::string& uuid, uint64_t value);
  std::map<std::string, uint64_t> GetAdsUUIDSeen();
  void ResetAdsUUIDSeenForAds(
      const std::vector<CategoryInfo>& categories);
  void SetAvailable(const bool available);
  void SetAllowed(const bool allowed);
  bool GetAllowed();
  void SetConfigured(const bool configured);
  bool GetConfigured();
  void SetCurrentSSID(const std::string& ssid);
  void SetExpired(const bool expired);
  void FlagShoppingState(const std::string& url, const uint64_t score);
  void UnflagShoppingState();
  bool GetShoppingState();
  void FlagSearchState(const std::string& url, const uint64_t score);
  void UnflagSearchState(const std::string &url);
  bool GetSearchState();
  void UpdateLastUserActivity();
  void UpdateLastUserIdleStopTime();
  void SetLocale(const std::string& locale);
  std::string GetLocale();
  void SetLocales(const std::vector<std::string>& locales);
  std::vector<std::string> GetLocales();
  void AppendPageScoreToPageScoreHistory(
      const std::vector<double>& page_scores);
  std::deque<std::vector<double>> GetPageScoreHistory();
  std::string GetCurrentPlace();

  void RemoveAllHistory();

 private:
  void OnClientSaved(const ads::Result result);

  ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<CLIENT_STATE> client_state_;
};

}  // namespace ads
