/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/credentials/credentials_sku.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

using GetContributionInfoAndUnblindedTokensCallback = std::function<void(
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::UnblindedToken>& unblinded_tokens)>;

using StatisticalVotingWinners = std::map<std::string, uint32_t>;

class Unblinded {
 public:
  explicit Unblinded(RewardsEngineImpl& engine);
  ~Unblinded();

  void Start(const std::vector<mojom::CredsBatchType>& types,
             const std::string& contribution_id,
             LegacyResultCallback callback);

  void Retry(const std::vector<mojom::CredsBatchType>& types,
             mojom::ContributionInfoPtr contribution,
             LegacyResultCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(UnblindedTest, GetStatisticalVotingWinner);

  void GetContributionInfoAndUnblindedTokens(
      const std::vector<mojom::CredsBatchType>& types,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnUnblindedTokens(
      std::vector<mojom::UnblindedTokenPtr> unblinded_tokens,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void GetContributionInfoAndReservedUnblindedTokens(
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnReservedUnblindedTokens(
      std::vector<mojom::UnblindedTokenPtr> unblinded_tokens,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnGetContributionInfo(
      mojom::ContributionInfoPtr contribution,
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void PrepareTokens(mojom::ContributionInfoPtr contribution,
                     const std::vector<mojom::UnblindedToken>& unblinded_tokens,
                     const std::vector<mojom::CredsBatchType>& types,
                     LegacyResultCallback callback);

  void PreparePublishers(
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      mojom::ContributionInfoPtr contribution,
      const std::vector<mojom::CredsBatchType>& types,
      LegacyResultCallback callback);

  std::vector<mojom::ContributionPublisherPtr> PrepareAutoContribution(
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      mojom::ContributionInfoPtr contribution);

  void OnPrepareAutoContribution(
      mojom::Result result,
      const std::vector<mojom::CredsBatchType>& types,
      const std::string& contribution_id,
      LegacyResultCallback callback);

  void PrepareStepSaved(mojom::Result result,
                        const std::vector<mojom::CredsBatchType>& types,
                        const std::string& contribution_id,
                        LegacyResultCallback callback);

  void ProcessTokens(const std::vector<mojom::CredsBatchType>& types,
                     const std::string& contribution_id,
                     LegacyResultCallback callback);

  void OnProcessTokens(
      mojom::ContributionInfoPtr contribution,
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      LegacyResultCallback callback);

  void TokenProcessed(mojom::Result result,
                      const std::string& contribution_id,
                      const std::string& publisher_key,
                      bool final_publisher,
                      LegacyResultCallback callback);

  void ContributionAmountSaved(mojom::Result result,
                               const std::string& contribution_id,
                               bool final_publisher,
                               LegacyResultCallback callback);

  void OnMarkUnblindedTokensAsReserved(
      mojom::Result result,
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
      const std::vector<mojom::CredsBatchType>& types,
      LegacyResultCallback callback);

  void OnReservedUnblindedTokensForRetryAttempt(
      const std::vector<mojom::UnblindedTokenPtr>& unblinded_tokens,
      const std::vector<mojom::CredsBatchType>& types,
      std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
      LegacyResultCallback callback);

  std::string GetStatisticalVotingWinnerForTesting(
      double dart,
      double amount,
      const std::vector<mojom::ContributionPublisherPtr>& publisher_list);

  const raw_ref<RewardsEngineImpl> engine_;
  credential::CredentialsPromotion credentials_promotion_;
  credential::CredentialsSKU credentials_sku_;
};

}  // namespace contribution
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
