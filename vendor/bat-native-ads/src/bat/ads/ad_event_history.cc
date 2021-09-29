/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_event_history.h"

#include "base/check.h"
#include "base/time/time.h"

namespace ads {

namespace {

std::string GetId(const std::string& ad_type,
                  const std::string& confirmation_type) {
  return ad_type + confirmation_type;
}

void PurgeHistoryOlderThan(std::vector<double>* history,
                           const base::TimeDelta& time_delta) {
  DCHECK(history);

  const base::Time past = base::Time::Now() - time_delta;

  const auto iter = std::remove_if(
      history->begin(), history->end(), [&past](const double timestamp) {
        return base::Time::FromDoubleT(timestamp) < past;
      });

  history->erase(iter, history->end());
}

}  // namespace

AdEventHistory::AdEventHistory() = default;

AdEventHistory::~AdEventHistory() = default;

void AdEventHistory::Record(const std::string& ad_type,
                            const std::string& confirmation_type,
                            const double timestamp) {
  const std::string id = GetId(ad_type, confirmation_type);
  DCHECK(!id.empty());

  const auto iter = history_.find(id);
  if (iter == history_.end()) {
    history_.insert({id, {timestamp}});
    return;
  }

  iter->second.push_back(timestamp);

  const base::TimeDelta time_delta = base::TimeDelta::FromDays(1);
  PurgeHistoryOlderThan(&iter->second, time_delta);
}

std::vector<double> AdEventHistory::Get(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  const std::string id = GetId(ad_type, confirmation_type);
  DCHECK(!id.empty());

  const auto iter = history_.find(id);
  if (iter == history_.end()) {
    return {};
  }

  return iter->second;
}

void AdEventHistory::Reset() {
  history_ = {};
}

}  // namespace ads
