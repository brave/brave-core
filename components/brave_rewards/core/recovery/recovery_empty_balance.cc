/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/recovery/recovery_empty_balance.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "net/http/http_status_code.h"

namespace {

const int32_t kVersion = 1;

}  // namespace

namespace brave_rewards::internal {
namespace recovery {

EmptyBalance::EmptyBalance(RewardsEngineImpl& engine)
    : engine_(engine), promotion_server_(engine) {}

EmptyBalance::~EmptyBalance() = default;

void EmptyBalance::Check() {
  engine_->database()->GetAllContributions(base::BindOnce(
      &EmptyBalance::OnAllContributions, weak_factory_.GetWeakPtr()));
}

void EmptyBalance::OnAllContributions(
    std::vector<mojom::ContributionInfoPtr> list) {
  // we can just restore all tokens if no contributions
  if (list.empty()) {
    GetPromotions(base::BindOnce(&EmptyBalance::GetCredsByPromotions,
                                 weak_factory_.GetWeakPtr()));
    return;
  }

  double contribution_sum = 0.0;
  for (const auto& contribution : list) {
    if (contribution->step == mojom::ContributionStep::STEP_COMPLETED) {
      contribution_sum += contribution->amount;
    }
  }

  engine_->Log(FROM_HERE) << "Contribution SUM: " << contribution_sum;

  GetPromotions(base::BindOnce(&EmptyBalance::GetAllTokens,
                               weak_factory_.GetWeakPtr(), contribution_sum));
}

void EmptyBalance::GetPromotions(database::GetPromotionListCallback callback) {
  engine_->database()->GetAllPromotions(
      base::BindOnce(&EmptyBalance::OnPromotions, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void EmptyBalance::OnPromotions(
    database::GetPromotionListCallback callback,
    base::flat_map<std::string, mojom::PromotionPtr> promotions) {
  std::vector<mojom::PromotionPtr> list;

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    if (promotion.second->status == mojom::PromotionStatus::FINISHED &&
        promotion.second->type == mojom::PromotionType::ADS) {
      list.push_back(std::move(promotion.second));
    }
  }

  std::move(callback).Run(std::move(list));
}

void EmptyBalance::GetCredsByPromotions(std::vector<mojom::PromotionPtr> list) {
  std::vector<std::string> promotion_ids;
  for (auto& promotion : list) {
    promotion_ids.push_back(promotion->id);
  }

  engine_->database()->GetCredsBatchesByTriggers(
      promotion_ids,
      base::BindOnce(&EmptyBalance::OnCreds, weak_factory_.GetWeakPtr()));
}

void EmptyBalance::OnCreds(std::vector<mojom::CredsBatchPtr> list) {
  if (list.empty()) {
    engine_->Log(FROM_HERE) << "Creds batch list is emtpy";
    engine_->state()->SetEmptyBalanceChecked(true);
    return;
  }

  std::string error;
  std::vector<mojom::UnblindedTokenPtr> token_list;
  mojom::UnblindedTokenPtr unblinded;
  const uint64_t expires_at = 0ul;
  for (auto& creds_batch : list) {
    auto unblinded_encoded_creds = credential::UnBlindCreds(*creds_batch);

    if (!unblinded_encoded_creds.has_value()) {
      engine_->LogError(FROM_HERE)
          << "UnBlindTokens: " << std::move(unblinded_encoded_creds).error();
      continue;
    }

    for (auto& cred : *unblinded_encoded_creds) {
      unblinded = mojom::UnblindedToken::New();
      unblinded->token_value = cred;
      unblinded->public_key = creds_batch->public_key;
      unblinded->value = 0.25;
      unblinded->creds_id = creds_batch->creds_id;
      unblinded->expires_at = expires_at;
      token_list.push_back(std::move(unblinded));
    }
  }

  if (token_list.empty()) {
    engine_->Log(FROM_HERE) << "Unblinded token list is emtpy";
    engine_->state()->SetEmptyBalanceChecked(true);
    return;
  }

  engine_->database()->SaveUnblindedTokenList(
      std::move(token_list), base::BindOnce(&EmptyBalance::OnSaveUnblindedCreds,
                                            weak_factory_.GetWeakPtr()));
}

void EmptyBalance::OnSaveUnblindedCreds(const mojom::Result result) {
  engine_->Log(FROM_HERE) << "Finished empty balance migration with result: "
                          << result;
  engine_->state()->SetEmptyBalanceChecked(true);
}

void EmptyBalance::GetAllTokens(double contribution_sum,
                                std::vector<mojom::PromotionPtr> list) {
  // from all completed promotions get creds
  // unblind them and save them
  double promotion_sum = 0.0;
  for (auto& promotion : list) {
    promotion_sum += promotion->approximate_value;
  }

  engine_->Log(FROM_HERE) << "Promotion SUM: " << promotion_sum;

  engine_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {mojom::CredsBatchType::PROMOTION},
      base::BindOnce(&EmptyBalance::ReportResults, weak_factory_.GetWeakPtr(),
                     contribution_sum, promotion_sum));
}

void EmptyBalance::ReportResults(double contribution_sum,
                                 double promotion_sum,
                                 std::vector<mojom::UnblindedTokenPtr> list) {
  double tokens_sum = 0.0;
  for (auto& item : list) {
    tokens_sum += item->value;
  }
  engine_->Log(FROM_HERE) << "Token SUM: " << tokens_sum;

  double total = promotion_sum - contribution_sum - tokens_sum;

  if (total <= 0) {
    engine_->Log(FROM_HERE) << "Unblinded token total is OK";
    engine_->state()->SetEmptyBalanceChecked(true);
    return;
  }

  engine_->Log(FROM_HERE) << "Unblinded token total is " << total;

  promotion_server_.post_bat_loss().Request(
      total, kVersion,
      base::BindOnce(&EmptyBalance::Sent, weak_factory_.GetWeakPtr()));
}

void EmptyBalance::Sent(mojom::Result result) {
  if (result != mojom::Result::OK) {
    return;
  }

  engine_->Log(FROM_HERE) << "Finished empty balance migration";
  engine_->state()->SetEmptyBalanceChecked(true);
}

}  // namespace recovery
}  // namespace brave_rewards::internal
