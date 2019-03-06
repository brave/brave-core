/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_STATE_H_
#define BAT_ADS_INTERNAL_CLIENT_STATE_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <deque>

#include "bat/ads/result.h"

namespace ads {

struct ClientState {
  ClientState();
  explicit ClientState(const ClientState& state);
  ~ClientState();

  const std::string ToJson();
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::deque<uint64_t> ads_shown_history;
  std::string ad_uuid;
  std::map<std::string, uint64_t> ads_uuid_seen;
  bool available;
  uint64_t last_search_time;
  uint64_t last_shop_time;
  uint64_t last_user_activity;
  uint64_t last_user_idle_stop_time;
  std::string locale;
  std::vector<std::string> locales;
  std::string last_page_classification;
  std::deque<std::vector<double>> page_score_history;
  std::map<std::string, std::deque<uint64_t>> creative_set_history;
  std::map<std::string, std::deque<uint64_t>> campaign_history;
  double score;
  bool search_activity;
  std::string search_url;
  bool shop_activity;
  std::string shop_url;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_STATE_H_
