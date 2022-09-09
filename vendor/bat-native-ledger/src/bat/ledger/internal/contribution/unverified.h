/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_UNVERIFIED_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_UNVERIFIED_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/timer/timer.h"
#include "bat/ledger/ledger.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class Unverified {
 public:
  explicit Unverified(LedgerImpl* ledger);

  ~Unverified();

  void Contribute();

 private:
  void WasPublisherProcessed(const mojom::Result result,
                             const std::string& publisher_key,
                             const std::string& name);

  void ProcessedPublisherSaved(const mojom::Result result,
                               const std::string& publisher_key,
                               const std::string& name);

  void FetchInfoForUnverifiedPublishers(
      std::vector<std::string>&& publisher_keys);

  void OnRemovePendingContribution(mojom::Result result);

  void ProcessNext();

  void OnContributeUnverifiedBalance(mojom::Result result,
                                     mojom::BalancePtr properties);

  void OnContributeUnverifiedPublishers(
      double balance,
      const std::vector<mojom::PendingContributionInfoPtr>& list);

  void QueueSaved(const mojom::Result result,
                  const uint64_t pending_contribution_id);

  void ProcessingCompleted();

  LedgerImpl* ledger_;  // NOT OWNED
  base::OneShotTimer unverified_publishers_timer_;
  absl::optional<base::Time> processing_start_time_;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_UNVERIFIED_H_
