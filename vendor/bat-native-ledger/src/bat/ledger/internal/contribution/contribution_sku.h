/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_SKU_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_SKU_H_

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

  void AutoContribution(const std::string& contribution_id,
                        const std::string& wallet_type,
                        ledger::LegacyResultCallback callback);

  void Merchant(
      const type::SKUTransaction& transaction,
      client::TransactionCallback callback);

  void Retry(type::ContributionInfoPtr contribution,
             ledger::LegacyResultCallback callback);

 private:
  void Start(const std::string& contribution_id,
             const type::SKUOrderItem& item,
             const std::string& wallet_type,
             ledger::LegacyResultCallback callback);

  void GetContributionInfo(type::ContributionInfoPtr contribution,
                           const type::SKUOrderItem& item,
                           const std::string& wallet_type,
                           ledger::LegacyResultCallback callback);

  void GetOrder(type::Result result,
                const std::string& order_id,
                const std::string& contribution_id,
                ledger::LegacyResultCallback callback);

  void OnGetOrder(type::SKUOrderPtr order,
                  const std::string& contribution_id,
                  ledger::LegacyResultCallback callback);

  void Completed(type::Result result,
                 const std::string& contribution_id,
                 type::RewardsType type,
                 ledger::LegacyResultCallback callback);

  void CredsStepSaved(type::Result result,
                      const std::string& contribution_id,
                      ledger::LegacyResultCallback callback);

  void GetUnblindedTokens(
      type::UnblindedTokenList list,
      const type::SKUTransaction& transaction,
      client::TransactionCallback callback);

  void GetOrderMerchant(
      type::SKUOrderPtr order,
      const credential::CredentialsRedeem& redeem,
      client::TransactionCallback callback);

  void OnRedeemTokens(
      const type::Result result,
      client::TransactionCallback callback);

  void OnOrder(type::SKUOrderPtr order,
               std::shared_ptr<type::ContributionInfoPtr> shared_contribution,
               ledger::LegacyResultCallback callback);

  void RetryStartStep(type::ContributionInfoPtr contribution,
                      type::SKUOrderPtr order,
                      ledger::LegacyResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<credential::Credentials> credentials_;
  std::unique_ptr<sku::SKU> sku_;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_SKU_H_
