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
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/credentials/credentials_sku.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

class Unblinded {
 public:
  explicit Unblinded(RewardsEngineImpl& engine);
  ~Unblinded();

  void Start(const std::vector<mojom::CredsBatchType>& types,
             const std::string& contribution_id,
             ResultCallback callback);

  void Retry(const std::vector<mojom::CredsBatchType>& types,
             mojom::ContributionInfoPtr contribution,
             ResultCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(UnblindedTest, GetStatisticalVotingWinner);

  using GetContributionInfoAndUnblindedTokensCallback = base::OnceCallback<void(
      mojom::ContributionInfoPtr contribution,
      std::vector<mojom::UnblindedToken> unblinded_tokens)>;

  void GetContributionInfoAndUnblindedTokens(
      const std::vector<mojom::CredsBatchType>& types,
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnUnblindedTokens(
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback,
      std::vector<mojom::UnblindedTokenPtr> unblinded_tokens);

  void GetContributionInfoAndReservedUnblindedTokens(
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback);

  void OnReservedUnblindedTokens(
      const std::string& contribution_id,
      GetContributionInfoAndUnblindedTokensCallback callback,
      std::vector<mojom::UnblindedTokenPtr> unblinded_tokens);

  void OnGetContributionInfo(
      std::vector<mojom::UnblindedToken> unblinded_tokens,
      GetContributionInfoAndUnblindedTokensCallback callback,
      mojom::ContributionInfoPtr contribution);

  void PrepareTokens(std::vector<mojom::CredsBatchType> types,
                     ResultCallback callback,
                     mojom::ContributionInfoPtr contribution,
                     std::vector<mojom::UnblindedToken> unblinded_tokens);

  void PreparePublishers(
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      mojom::ContributionInfoPtr contribution,
      const std::vector<mojom::CredsBatchType>& types,
      ResultCallback callback);

  std::vector<mojom::ContributionPublisherPtr> PrepareAutoContribution(
      const std::vector<mojom::UnblindedToken>& unblinded_tokens,
      mojom::ContributionInfoPtr contribution);

  void OnPrepareAutoContribution(std::vector<mojom::CredsBatchType> types,
                                 const std::string& contribution_id,
                                 ResultCallback callback,
                                 mojom::Result result);

  void PrepareStepSaved(std::vector<mojom::CredsBatchType> types,
                        const std::string& contribution_id,
                        ResultCallback callback,
                        mojom::Result result);

  void ProcessTokens(const std::vector<mojom::CredsBatchType>& types,
                     const std::string& contribution_id,
                     ResultCallback callback);

  void OnProcessTokens(ResultCallback callback,
                       mojom::ContributionInfoPtr contribution,
                       std::vector<mojom::UnblindedToken> unblinded_tokens);

  void TokenProcessed(const std::string& contribution_id,
                      const std::string& publisher_key,
                      bool final_publisher,
                      ResultCallback callback,
                      mojom::Result result);

  void ContributionAmountSaved(const std::string& contribution_id,
                               bool final_publisher,
                               ResultCallback callback,
                               mojom::Result result);

  void OnMarkUnblindedTokensAsReserved(
      std::vector<mojom::UnblindedToken> unblinded_tokens,
      mojom::ContributionInfoPtr contribution,
      std::vector<mojom::CredsBatchType> types,
      ResultCallback callback,
      mojom::Result result);

  void OnReservedUnblindedTokensForRetryAttempt(
      std::vector<mojom::CredsBatchType> types,
      mojom::ContributionInfoPtr contribution,
      ResultCallback callback,
      std::vector<mojom::UnblindedTokenPtr> unblinded_tokens);

  std::string GetStatisticalVotingWinnerForTesting(
      double dart,
      double amount,
      const std::vector<mojom::ContributionPublisherPtr>& publisher_list);

  const raw_ref<RewardsEngineImpl> engine_;
  credential::CredentialsPromotion credentials_promotion_;
  credential::CredentialsSKU credentials_sku_;
  base::WeakPtrFactory<Unblinded> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_UNBLINDED_H_
