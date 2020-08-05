/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_CLIENT_STATE_H_
#define BAT_ADS_INTERNAL_CLIENT_CLIENT_STATE_H_

#include <stdint.h>

#include <deque>
#include <map>
#include <string>

#include "bat/ads/ad_history.h"
#include "bat/ads/internal/classification/page_classifier/page_classifier.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_history.h"
#include "bat/ads/internal/client/preferences/ad_preferences.h"
#include "bat/ads/result.h"

namespace ads {

struct ClientState {
  ClientState();
  ClientState(
      const ClientState& state);
  ~ClientState();

  std::string ToJson();
  Result FromJson(
      const std::string& json);

  AdPreferences ad_prefs;
  std::deque<AdHistory> ads_shown_history;
  std::string ad_uuid;
  std::map<std::string, uint64_t> seen_ad_notifications;
  std::map<std::string, uint64_t> seen_advertisers;
  uint64_t next_check_serve_ad_timestamp_in_seconds = 0;
  bool available = false;
  classification::PageProbabilitiesList page_probabilities_history;
  std::map<std::string, std::deque<uint64_t>> creative_set_history;
  std::map<std::string, std::deque<uint64_t>> ad_conversion_history;
  std::map<std::string, std::deque<uint64_t>> campaign_history;
  double score = 0.0;
  std::string version_code;
  PurchaseIntentSignalSegmentHistoryMap purchase_intent_signal_history;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_CLIENT_STATE_H_
