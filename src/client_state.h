/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <deque>

namespace ads {

struct ClientState {
  ClientState();
  explicit ClientState(const ClientState& state);
  ~ClientState();

  const std::string ToJson();
  bool FromJson(const std::string& json);

  std::deque<uint64_t> ads_shown_history;
  std::string ad_uuid;
  std::map<std::string, uint64_t> ads_uuid_seen;
  bool available;
  std::string current_ssid;
  bool expired;
  uint64_t last_search_time;
  uint64_t last_shop_time;
  uint64_t last_user_activity;
  uint64_t last_user_idle_stop_time;
  std::string locale;
  std::vector<std::string> locales;
  std::deque<std::vector<double>> page_score_history;
  std::map<std::string, std::deque<uint64_t>> creative_set_history;
  std::map<std::string, std::deque<uint64_t>> campaign_history;
  std::map<std::string, std::string> places;
  double score;
  bool search_activity;
  std::string search_url;
  bool shop_activity;
  std::string shop_url;
};

}  // namespace ads
