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

class Client: public CallbackHandler {
 public:
  Client(AdsImpl* ads, AdsClient* ads_client);
  ~Client();

  bool FromJson(const std::string& json);
  const std::string ToJson();

  void AppendCurrentTimeToAdsShownHistory();
  const std::deque<std::time_t> GetAdsShownHistory();
  void GetAdsShownHistory(const std::deque<std::time_t>& history);
  void UpdateAdUUID();
  void UpdateAdsUUIDSeen(const std::string& uuid, uint64_t value);
  const std::map<std::string, uint64_t> GetAdsUUIDSeen();
  void ResetAdsUUIDSeen(const std::vector<AdInfo>& ads);
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
  const std::string GetLocale();
  void SetLocales(const std::vector<std::string>& locales);
  const std::vector<std::string> GetLocales();
  void AppendPageScoreToPageScoreHistory(
      const std::vector<double>& page_scores);
  const std::deque<std::vector<double>> GetPageScoreHistory();
  const std::string GetCurrentPlace();

  void RemoveAllHistory();

 private:
  AdsImpl* ads_;  // NOT OWNED
  AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<CLIENT_STATE> client_state_;
};

}  // namespace ads
