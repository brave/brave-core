/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_SKU_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_SKU_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/credentials/credentials_sku.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/sku/sku.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionSKU {
 public:
  explicit ContributionSKU(LedgerImpl& ledger);
  ~ContributionSKU();

  void AutoContribution(const std::string& contribution_id,
                        const std::string& wallet_type,
                        ledger::LegacyResultCallback callback);

  void Merchant(const mojom::SKUTransaction&, LegacyResultCallback callback);

  void Retry(mojom::ContributionInfoPtr contribution,
             ledger::LegacyResultCallback callback);

 private:
  void Start(const std::string& contribution_id,
             const mojom::SKUOrderItem& item,
             const std::string& wallet_type,
             ledger::LegacyResultCallback callback);

  void GetContributionInfo(mojom::ContributionInfoPtr contribution,
                           const mojom::SKUOrderItem& item,
                           const std::string& wallet_type,
                           ledger::LegacyResultCallback callback);

  void GetOrder(mojom::Result result,
                const std::string& order_id,
                const std::string& contribution_id,
                ledger::LegacyResultCallback callback);

  void OnGetOrder(mojom::SKUOrderPtr order,
                  const std::string& contribution_id,
                  ledger::LegacyResultCallback callback);

  void Completed(mojom::Result result,
                 const std::string& contribution_id,
                 mojom::RewardsType type,
                 ledger::LegacyResultCallback callback);

  void CredsStepSaved(mojom::Result result,
                      const std::string& contribution_id,
                      ledger::LegacyResultCallback callback);

  void GetUnblindedTokens(std::vector<mojom::UnblindedTokenPtr> list,
                          const mojom::SKUTransaction&,
                          LegacyResultCallback);

  void GetOrderMerchant(mojom::SKUOrderPtr,
                        const credential::CredentialsRedeem&,
                        LegacyResultCallback);

  void OnRedeemTokens(mojom::Result, LegacyResultCallback);

  void OnOrder(mojom::SKUOrderPtr order,
               std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
               ledger::LegacyResultCallback callback);

  void RetryStartStep(mojom::ContributionInfoPtr contribution,
                      mojom::SKUOrderPtr order,
                      ledger::LegacyResultCallback callback);

  const raw_ref<LedgerImpl> ledger_;
  credential::CredentialsSKU credentials_;
  sku::SKU sku_;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_SKU_H_
