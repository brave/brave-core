/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_TIP_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_TIP_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionTip {
 public:
  explicit ContributionTip(LedgerImpl* ledger);

  ~ContributionTip();

  void Process(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

 private:
  void ServerPublisher(
      type::ServerPublisherInfoPtr server_info,
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

  void QueueSaved(
      const type::Result result,
      ledger::ResultCallback callback);

  void SavePending(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

  void OnSavePending(
      const type::Result result,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_TIP_H_
