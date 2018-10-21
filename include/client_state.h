/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <ctime>

namespace state {

struct CLIENT_STATE {
  CLIENT_STATE();
  explicit CLIENT_STATE(const CLIENT_STATE& state);
  ~CLIENT_STATE();

  bool LoadFromJson(const std::string& json);

  std::deque<std::time_t> ads_shown_history;
  std::string ad_uuid;
  std::map<std::string, uint64_t> ads_uuid_seen;
  bool available;
  bool allowed;
  bool configured;
  std::string current_ssid;
  bool expired;
  std::time_t last_search_time;
  std::time_t last_shop_time;
  std::time_t last_user_activity;
  std::time_t last_user_idle_stop_time;
  std::string locale;
  std::vector<std::string> locales;
  std::deque<std::vector<double>> page_score_history;
  std::map<std::string, std::string> places;
  double score;
  bool search_activity;
  std::string search_url;
  bool shop_activity;
  std::string shop_url;
  std::string status;
};

}  // namespace state
