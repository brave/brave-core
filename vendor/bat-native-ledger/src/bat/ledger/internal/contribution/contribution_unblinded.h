/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/credentials/credentials_factory.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

using GetContributionInfoAndUnblindedTokensCallback = std::function<void(
    type::ContributionInfoPtr contribution,
    const std::vector<type::UnblindedToken>& unblinded_tokens)>;

using StatisticalVotingWinners = std::map<std::string, uint32_t>;

class Unblinded {
 public:
  explicit Unblinded(LedgerImpl* ledger);
  ~Unblinded();

  void Start(
      const std::vector<type::CredsBatchType>& types,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void Retry(
      const std::vector<type::CredsBatchType>& types,
      type::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(UnblindedTest, GetStatisticalVotingWinner);

  void GetContributionInfoAndUnblindedTokens(
      const std::vector<type::CredsBatchType>& types,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnUnblindedTokens(
      type::UnblindedTokenList unblinded_tokens,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void GetContributionInfoAndReservedUnblindedTokens(
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnReservedUnblindedTokens(
      type::UnblindedTokenList unblinded_tokens,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnGetContributionInfo(
      type::ContributionInfoPtr contribution,
      const std::vector<type::UnblindedToken>& unblinded_tokens,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void PrepareTokens(type::ContributionInfoPtr contribution,
                     const std::vector<type::UnblindedToken>& unblinded_tokens,
                     const std::vector<type::CredsBatchType>& types,
                     ledger::ResultCallback callback);

  void PreparePublishers(
      const std::vector<type::UnblindedToken>& unblinded_tokens,
      type::ContributionInfoPtr contribution,
      const std::vector<type::CredsBatchType>& types,
      ledger::ResultCallback callback);

  type::ContributionPublisherList PrepareAutoContribution(
      const std::vector<type::UnblindedToken>& unblinded_tokens,
      type::ContributionInfoPtr contribution);

  void OnPrepareAutoContribution(
      const type::Result result,
      const std::vector<type::CredsBatchType>& types,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void PrepareStepSaved(
      const type::Result result,
      const std::vector<type::CredsBatchType>& types,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void ProcessTokens(
      const std::vector<type::CredsBatchType>& types,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void OnProcessTokens(
      type::ContributionInfoPtr contribution,
      const std::vector<type::UnblindedToken>& unblinded_tokens,
      ledger::ResultCallback callback);

  void TokenProcessed(
      const type::Result result,
      const std::string& contribution_id,
      const std::string& publisher_key,
      const bool final_publisher,
      ledger::ResultCallback callback);

  void ContributionAmountSaved(
      const type::Result result,
      const std::string& contribution_id,
      const bool final_publisher,
      ledger::ResultCallback callback);

  void OnMarkUnblindedTokensAsReserved(
      const type::Result result,
      const std::vector<type::UnblindedToken>& unblinded_tokens,
      std::shared_ptr<type::ContributionInfoPtr> shared_contribution,
      const std::vector<type::CredsBatchType>& types,
      ledger::ResultCallback callback);

  void OnReservedUnblindedTokensForRetryAttempt(
      const type::UnblindedTokenList& unblinded_tokens,
      const std::vector<type::CredsBatchType>& types,
      std::shared_ptr<type::ContributionInfoPtr> shared_contribution,
      ledger::ResultCallback callback);

  std::string GetStatisticalVotingWinnerForTesting(
      double dart,
      double amount,
      const ledger::type::ContributionPublisherList& publisher_list);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<credential::Credentials> credentials_promotion_;
  std::unique_ptr<credential::Credentials> credentials_sku_;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
