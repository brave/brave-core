/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_event_history.h"

#include "base/check.h"
#include "base/time/time.h"

namespace ads {

namespace {

std::string GetTypeId(const std::string& ad_type,
                      const std::string& confirmation_type) {
  return ad_type + confirmation_type;
}

void PurgeHistoryOlderThan(std::vector<double>* history,
                           const base::TimeDelta& time_delta) {
  DCHECK(history);

  const base::Time& past = base::Time::Now() - time_delta;

  const auto iter = std::remove_if(
      history->begin(), history->end(), [&past](const double timestamp) {
        return base::Time::FromDoubleT(timestamp) < past;
      });

  history->erase(iter, history->end());
}

}  // namespace

AdEventHistory::AdEventHistory() = default;

AdEventHistory::~AdEventHistory() = default;

void AdEventHistory::RecordForId(const std::string& id,
                                 const std::string& ad_type,
                                 const std::string& confirmation_type,
                                 const double timestamp) {
  DCHECK(!id.empty());
  DCHECK(!ad_type.empty());
  DCHECK(!confirmation_type.empty());

  const std::string& type_id = GetTypeId(ad_type, confirmation_type);

  history_[id][type_id].push_back(timestamp);

  const base::TimeDelta time_delta = base::Days(1);
  PurgeHistoryOlderThan(&history_[id][type_id], time_delta);
}

std::vector<double> AdEventHistory::Get(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  DCHECK(!ad_type.empty());
  DCHECK(!confirmation_type.empty());

  const std::string& type_id = GetTypeId(ad_type, confirmation_type);

  std::vector<double> timestamps;

  for (const auto& history : history_) {
    const std::map<std::string, std::vector<double>>& ad_events =
        history.second;

    for (const auto& ad_event : ad_events) {
      const std::string& ad_event_type_id = ad_event.first;
      if (ad_event_type_id != type_id) {
        continue;
      }

      const std::vector<double>& ad_event_timestamps = ad_event.second;

      timestamps.insert(timestamps.end(), ad_event_timestamps.cbegin(),
                        ad_event_timestamps.cend());
    }
  }

  return timestamps;
}

void AdEventHistory::ResetForId(const std::string& id) {
  history_[id] = {};
}

}  // namespace ads
