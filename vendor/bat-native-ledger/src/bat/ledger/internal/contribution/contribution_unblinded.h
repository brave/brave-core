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

using SendTokensCallback =
    std::function<void(const ledger::Result)>;

using Winners = std::map<std::string, uint32_t>;

class Unblinded {
 public:
  explicit Unblinded(bat_ledger::LedgerImpl* ledger);
  ~Unblinded();

  void Start(const std::string& viewing_id);

 private:
  void OnUnblindedTokens(
      const std::string& viewing_id,
      ledger::UnblindedTokenList list);

  void MakeContribution(
      const std::string& viewing_id,
      ledger::UnblindedTokenList list);

  bool GetStatisticalVotingWinner(
      double dart,
      const braveledger_bat_helper::Directions& directions,
      Winners* winners) const;

  void PrepareAutoContribution(
      const std::string& viewing_id,
      ledger::UnblindedTokenList list);

  void GetStatisticalVotingWinners(
      uint32_t total_votes,
      const braveledger_bat_helper::Directions& directions,
      Winners* winners) const;

  void SendTokens(
      const std::string& publisher_key,
      const ledger::RewardsType type,
      ledger::UnblindedTokenList list,
      SendTokensCallback callback);

  void OnSendTokens(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::vector<std::string>& token_id_list,
      SendTokensCallback callback);

  void ContributionCompleted(
      const ledger::Result result,
      const std::string& viewing_id);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
