/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_SKU_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_SKU_H_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/credentials/credentials_factory.h"
#include "bat/ledger/internal/sku/sku_factory.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionSKU {
 public:
  explicit ContributionSKU(LedgerImpl* ledger);
  ~ContributionSKU();

  void AutoContribution(
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

  void AnonUserFunds(
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

  void Merchant(
      const ledger::SKUTransaction& transaction,
      client::TransactionCallback callback);

  void Retry(
      const ledger::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

 private:
  void Start(
      const std::string& contribution_id,
      const ledger::SKUOrderItem& item,
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

  void GetContributionInfo(
      ledger::ContributionInfoPtr contribution,
      const ledger::SKUOrderItem& item,
      const ledger::ExternalWallet& wallet,
      ledger::ResultCallback callback);

  void GetOrder(
      const ledger::Result result,
      const std::string& order_id,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void OnGetOrder(
      ledger::SKUOrderPtr order,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void TransactionStepSaved(
      const ledger::Result result,
      std::shared_ptr<ledger::SKUOrderPtr> shared_order,
      ledger::ResultCallback callback);

  void Completed(
      const ledger::Result result,
      const std::string& contribution_id,
      const ledger::RewardsType type,
      ledger::ResultCallback callback);

  void CredsStepSaved(
      const ledger::Result result,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void GetUnblindedTokens(
      ledger::UnblindedTokenList list,
      const ledger::SKUTransaction& transaction,
      client::TransactionCallback callback);

  void GetOrderMerchant(
      ledger::SKUOrderPtr order,
      const credential::CredentialsRedeem& redeem,
      client::TransactionCallback callback);

  void OnRedeemTokens(
      const ledger::Result result,
      client::TransactionCallback callback);

  void OnOrder(
      ledger::SKUOrderPtr order,
      std::shared_ptr<ledger::ContributionInfoPtr> shared_contribution,
      ledger::ResultCallback callback);

  void RetryStartStep(
      ledger::ContributionInfoPtr contribution,
      ledger::SKUOrderPtr order,
      ledger::ResultCallback callback);

  void RetryStartStepExternalWallet(
      const ledger::Result result,
      ledger::ExternalWalletPtr wallet,
      const std::string& order_id,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void RetryExternalTransactionStep(
      ledger::ContributionInfoPtr contribution,
      ledger::SKUOrderPtr order,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<credential::Credentials> credentials_;
  std::unique_ptr<sku::SKU> sku_;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_SKU_H_
