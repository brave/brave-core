/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_AUTO_CONTRIBUTE_CALCULATOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_AUTO_CONTRIBUTE_CALCULATOR_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

namespace ledger {

class AutoContributeCalculator : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "auto-contribute-calculator";

  using WeightMap = base::flat_map<std::string, double>;
  using VoteMap = base::flat_map<std::string, size_t>;

  WeightMap CalculateWeights(const std::vector<PublisherActivity>& publishers,
                             int min_visits,
                             base::TimeDelta min_duration);

  VoteMap AllocateVotes(const WeightMap& publisher_weights, size_t total_votes);

  static double ConvertSecondsToScore(double seconds, double min_seconds);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_AUTO_CONTRIBUTE_CALCULATOR_H_
