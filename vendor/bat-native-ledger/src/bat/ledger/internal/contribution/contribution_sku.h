/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_SKU_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_SKU_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/credentials/credentials_factory.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class ContributionSKU {
 public:
  explicit ContributionSKU(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution);
  ~ContributionSKU();

  void AutoContribution(
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet);

  void AnonUserFunds(
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet);

  void Merchant(
      const ledger::SKUTransaction& transaction,
      const std::string& destination,
      ledger::TransactionCallback callback);

 private:
  void Start(
      const std::string& contribution_id,
      const ledger::SKUOrderItem& item,
      const std::string& destination,
      ledger::ExternalWalletPtr wallet);

  void GetContributionInfo(
      ledger::ContributionInfoPtr contribution,
      const ledger::SKUOrderItem& item,
      const std::string& destination,
      const ledger::ExternalWallet& wallet);

  void GetOrder(
      const ledger::Result result,
      const std::string& order_id,
      ledger::ResultCallback callback);

  void OnGetOrder(
      ledger::SKUOrderPtr order,
      ledger::ResultCallback callback);

  void Completed(
      const ledger::Result result,
      const std::string& contribution_id,
      const ledger::RewardsType type);

  void GetUnblindedTokens(
      ledger::UnblindedTokenList list,
      const ledger::SKUTransaction& transaction,
      const std::string& destination,
      ledger::TransactionCallback callback);

  void GerOrderMerchant(
      ledger::SKUOrderPtr order,
      const braveledger_credentials::CredentialsRedeem& redeem,
      ledger::TransactionCallback callback);

  void OnRedeemTokens(
      const ledger::Result result,
      ledger::TransactionCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
  std::unique_ptr<braveledger_credentials::Credentials> credentials_;
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_SKU_H_
