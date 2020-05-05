/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_H_
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_H_

#include <stdint.h>
#include <deque>
#include <string>

namespace ads {

class Client;

class FrequencyCapping {
 public:
  explicit FrequencyCapping(
      const Client* const client);

  ~FrequencyCapping();

  bool DoesHistoryRespectCapForRollingTimeConstraint(
      const std::deque<uint64_t> history,
      const uint64_t time_constraint_in_seconds,
      const uint64_t cap) const;

  std::deque<uint64_t> GetCreativeSetHistory(
      const std::string& creative_set_id) const;

  std::deque<uint64_t> GetAdsShownHistory() const;

  std::deque<uint64_t> GetAdsHistory(
      const std::string& creative_instance_id) const;

  std::deque<uint64_t> GetCampaign(
      const std::string& campaign_id) const;

  std::deque<uint64_t> GetAdConversionHistory(
      const std::string& creative_set_id) const;

 private:
  const Client* const client_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_H_
