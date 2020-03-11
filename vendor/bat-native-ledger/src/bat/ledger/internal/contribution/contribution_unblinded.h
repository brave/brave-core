/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

using GetContributionInfoAndUnblindedTokensCallback = std::function<void(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::UnblindedToken>& list)>;

using Winners = std::map<std::string, uint32_t>;

class Unblinded {
 public:
  explicit Unblinded(bat_ledger::LedgerImpl* ledger);
  ~Unblinded();

  void Initialize();

  void Start(const std::string& contribution_id);

  void OnTimer(uint32_t timer_id);

 private:
  void OnGetNotCompletedContributions(
      ledger::ContributionInfoList list);

  void GetContributionInfoAndUnblindedTokens(
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnUnblindedTokens(
      ledger::UnblindedTokenList list,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnGetContributionInfo(
      ledger::ContributionInfoPtr contribution,
      const std::vector<ledger::UnblindedToken>& list,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void PrepareTokens(
      ledger::ContributionInfoPtr contribution,
      const std::vector<ledger::UnblindedToken>& list);

  void PreparePublishers(
      const std::vector<ledger::UnblindedToken>& list,
      ledger::ContributionInfoPtr contribution);

  ledger::ContributionPublisherList PrepareAutoContribution(
      const std::vector<ledger::UnblindedToken>& list,
      ledger::ContributionInfoPtr contribution);

  void OnPrepareAutoContribution(
      const ledger::Result result,
      const std::string& contribution_id);

  void ProcessTokens(const std::string& contribution_id);

  void OnProcessTokens(
      ledger::ContributionInfoPtr contribution,
      const std::vector<ledger::UnblindedToken>& list);

  void TokenProcessed(
      const ledger::Result result,
      const std::string& contribution_id,
      const std::string& publisher_key);

  void OnTokenProcessed(
      const ledger::Result result,
      const std::string& contribution_id);

  void CheckIfCompleted(ledger::ContributionInfoPtr contribution);

  void SendTokens(
      const std::string& publisher_key,
      ledger::ContributionInfoPtr contribution,
      const std::vector<ledger::UnblindedToken>& list,
      ledger::ResultCallback callback);

  void OnSendTokens(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::vector<std::string>& token_id_list,
      ledger::ResultCallback callback);

  void ContributionCompleted(
      const ledger::Result result,
      ledger::ContributionInfoPtr contribution);

  void SetTimer(
      const std::string& contribution_id,
      const uint64_t& start_timer_in = 0);

  void CheckStep(const std::string& contribution_id);

  void DoRetry(ledger::ContributionInfoPtr contribution);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, uint32_t> retry_timers_;
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
