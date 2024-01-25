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
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/credentials/credentials_sku.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/sku/sku.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

class ContributionSKU {
 public:
  explicit ContributionSKU(RewardsEngineImpl& engine);
  ~ContributionSKU();

  void AutoContribution(const std::string& contribution_id,
                        const std::string& wallet_type,
                        ResultCallback callback);

  void Merchant(const mojom::SKUTransaction&, ResultCallback callback);

  void Retry(mojom::ContributionInfoPtr contribution, ResultCallback callback);

 private:
  void Start(const std::string& contribution_id,
             const mojom::SKUOrderItem& item,
             const std::string& wallet_type,
             ResultCallback callback);

  void GetContributionInfo(const mojom::SKUOrderItem& item,
                           const std::string& wallet_type,
                           ResultCallback callback,
                           mojom::ContributionInfoPtr contribution);

  void GetOrder(const std::string& contribution_id,
                ResultCallback callback,
                mojom::Result result,
                const std::string& order_id);

  void OnGetOrder(const std::string& contribution_id,
                  ResultCallback callback,
                  mojom::SKUOrderPtr order);

  void Completed(const std::string& contribution_id,
                 mojom::RewardsType type,
                 ResultCallback callback,
                 mojom::Result result);

  void CredsStepSaved(const std::string& contribution_id,
                      ResultCallback callback,
                      mojom::Result result);

  void GetUnblindedTokens(const mojom::SKUTransaction&,
                          ResultCallback,
                          std::vector<mojom::UnblindedTokenPtr> list);

  void GetOrderMerchant(const credential::CredentialsRedeem&,
                        ResultCallback,
                        mojom::SKUOrderPtr);

  void OnRedeemTokens(ResultCallback, mojom::Result);

  void OnOrder(mojom::ContributionInfoPtr contribution,
               ResultCallback callback,
               mojom::SKUOrderPtr order);

  void RetryStartStep(mojom::ContributionInfoPtr contribution,
                      mojom::SKUOrderPtr order,
                      ResultCallback callback);

  const raw_ref<RewardsEngineImpl> engine_;
  credential::CredentialsSKU credentials_;
  sku::SKU sku_;
  base::WeakPtrFactory<ContributionSKU> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_SKU_H_
