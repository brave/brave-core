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

#include "ads_impl.h"
#include "ads_client.h"
#include "callback_handler.h"
#include "client_state.h"

namespace rewards_ads {
class AdsImpl;
}  // namespace rewards_ads

namespace state {

class Client: public ads::CallbackHandler {
 public:
  Client(rewards_ads::AdsImpl* ads, ads::AdsClient* ads_client);
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
      const std::vector<bundle::CategoryInfo>& categories);
  void SetAvailable(const bool available);
  void SetAllowed(const bool allowed);
  void SetConfigured(const bool configured);
  void SetCurrentSSID(const std::string& ssid);
  void SetExpired(const bool expired);
  void FlagShoppingState(const std::string& url, const uint64_t score);
  void UnflagShoppingState();
  void FlagSearchState(const std::string& url, const uint64_t score);
  void UnflagSearchState(const std::string &url);
  void UpdateLastUserActivity();
  void UpdateLastUserIdleStopTime();
  void SetLocale(const std::string& locale);
  std::string GetLocale();
  void SetLocales(const std::vector<std::string>& locales);
  std::vector<std::string> GetLocales();
  void AppendPageScoreToPageScoreHistory(
      const std::vector<double>& page_scores);
  std::deque<std::vector<double>> GetPageScoreHistory();
  void RemoveAllHistory();

 private:
  void OnClientSaved(const ads::Result result);

  rewards_ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<CLIENT_STATE> client_state_;
};

}  // namespace state
