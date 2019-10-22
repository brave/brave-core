/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_PHASE_ONE_H_
#define BRAVELEDGER_CONTRIBUTION_PHASE_ONE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/contribution/contribution.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class PhaseOne {
 public:
  explicit PhaseOne(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution);

  ~PhaseOne();

  void Start(const std::string &viewing_id);

  void CurrentReconcile(const std::string& viewing_id);

  void ReconcilePayload(const std::string& viewing_id);

  void RegisterViewing(const std::string& viewing_id);

  void ViewingCredentials(const std::string& viewing_id);

  void Complete(ledger::Result result,
                const std::string& viewing_id,
                const ledger::RewardsType type,
                const std::string& probi = "0");

 private:
  std::string GetAnonizeProof(const std::string& registrar_VK,
                              const std::string& id,
                              std::string* pre_flight);

  void ReconcileCallback(const std::string& viewing_id,
                         int response_status_code,
                         const std::string& response,
                         const std::map<std::string, std::string>& headers);

  void CurrentReconcileCallback(
      const std::string& viewing_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void ReconcilePayloadCallback(
      const std::string& viewing_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void RegisterViewingCallback(
      const std::string& viewing_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void ViewingCredentialsCallback(
      const std::string& viewing_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_PHASE_ONE_H_
