/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads/ad_event/ad_event_history.h"

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/containers/flat_map.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"

namespace brave_ads {

namespace {

std::string GetTypeId(const std::string& ad_type,
                      const std::string& confirmation_type) {
  return base::StrCat({ad_type, confirmation_type});
}

void PurgeHistoryOlderThan(std::vector<base::Time>* history,
                           const base::TimeDelta time_delta) {
  CHECK(history);

  const base::Time past = base::Time::Now() - time_delta;

  history->erase(
      base::ranges::remove_if(
          *history, [past](const base::Time time) { return time < past; }),
      history->end());
}

}  // namespace

AdEventHistory::AdEventHistory() = default;

AdEventHistory::~AdEventHistory() = default;

void AdEventHistory::RecordForId(const std::string& id,
                                 const std::string& ad_type,
                                 const std::string& confirmation_type,
                                 const base::Time time) {
  CHECK(!id.empty());
  CHECK(!ad_type.empty());
  CHECK(!confirmation_type.empty());

  const std::string type_id = GetTypeId(ad_type, confirmation_type);

  history_[id][type_id].push_back(time);

  PurgeHistoryOlderThan(&history_[id][type_id], base::Days(1));
}

std::vector<base::Time> AdEventHistory::Get(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  CHECK(!ad_type.empty());
  CHECK(!confirmation_type.empty());

  const std::string type_id = GetTypeId(ad_type, confirmation_type);

  std::vector<base::Time> timestamps;

  for (const auto& [uuid, ad_events] : history_) {
    for (const auto& [ad_event_type_id, ad_event_timestamps] : ad_events) {
      if (ad_event_type_id == type_id) {
        base::Extend(timestamps, ad_event_timestamps);
      }
    }
  }

  return timestamps;
}

void AdEventHistory::ResetForId(const std::string& id) {
  history_[id] = {};
}

}  // namespace brave_ads
