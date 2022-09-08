/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_AC_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_AC_H_

#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionAC {
 public:
  explicit ContributionAC(LedgerImpl* ledger);

  ~ContributionAC();

  void Process(const uint64_t reconcile_stamp);

 private:
  void PreparePublisherList(std::vector<mojom::PublisherInfoPtr> list);

  void QueueSaved(const mojom::Result result);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_AC_H_
