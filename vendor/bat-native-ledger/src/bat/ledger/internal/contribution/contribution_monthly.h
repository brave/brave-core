/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_

#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionMonthly {
 public:
  explicit ContributionMonthly(LedgerImpl* ledger);

  ~ContributionMonthly();

  void Process(ledger::LegacyResultCallback callback);

 private:
  void PrepareTipList(std::vector<mojom::PublisherInfoPtr> list,
                      ledger::LegacyResultCallback callback);

  void GetVerifiedTipList(const std::vector<mojom::PublisherInfoPtr>& list,
                          std::vector<mojom::PublisherInfoPtr>* verified_list);

  void OnSavePendingContribution(const mojom::Result result);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
