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

using std::placeholders::_1;

namespace {

const int32_t kVersion = 1;

}  // namespace

namespace brave_rewards::internal {
namespace recovery {

EmptyBalance::EmptyBalance(RewardsEngineImpl& engine)
    : engine_(engine), promotion_server_(engine) {}

EmptyBalance::~EmptyBalance() = default;

void EmptyBalance::Check() {
  auto get_callback = std::bind(&EmptyBalance::OnAllContributions, this, _1);
  engine_->database()->GetAllContributions(get_callback);
}

void EmptyBalance::OnAllContributions(
    std::vector<mojom::ContributionInfoPtr> list) {
  // we can just restore all tokens if no contributions
  if (list.empty()) {
    auto get_callback =
        std::bind(&EmptyBalance::GetCredsByPromotions, this, _1);

    GetPromotions(get_callback);
    return;
  }

  double contribution_sum = 0.0;
  for (const auto& contribution : list) {
    if (contribution->step == mojom::ContributionStep::STEP_COMPLETED) {
      contribution_sum += contribution->amount;
    }
  }

  engine_->Log(FROM_HERE) << "Contribution SUM: " << contribution_sum;

  auto get_callback =
      std::bind(&EmptyBalance::GetAllTokens, this, _1, contribution_sum);

  GetPromotions(get_callback);
}

void EmptyBalance::GetPromotions(database::GetPromotionListCallback callback) {
  auto get_callback =
      std::bind(&EmptyBalance::OnPromotions, this, _1, callback);

  engine_->database()->GetAllPromotions(get_callback);
}

void EmptyBalance::OnPromotions(
    base::flat_map<std::string, mojom::PromotionPtr> promotions,
    database::GetPromotionListCallback callback) {
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

  callback(std::move(list));
}

void EmptyBalance::GetCredsByPromotions(std::vector<mojom::PromotionPtr> list) {
  std::vector<std::string> promotion_ids;
  for (auto& promotion : list) {
    promotion_ids.push_back(promotion->id);
  }

  auto get_callback = std::bind(&EmptyBalance::OnCreds, this, _1);

  engine_->database()->GetCredsBatchesByTriggers(promotion_ids, get_callback);
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

  auto save_callback = std::bind(&EmptyBalance::OnSaveUnblindedCreds, this, _1);

  engine_->database()->SaveUnblindedTokenList(std::move(token_list),
                                              save_callback);
}

void EmptyBalance::OnSaveUnblindedCreds(const mojom::Result result) {
  engine_->Log(FROM_HERE) << "Finished empty balance migration with result: "
                          << result;
  engine_->state()->SetEmptyBalanceChecked(true);
}

void EmptyBalance::GetAllTokens(std::vector<mojom::PromotionPtr> list,
                                const double contribution_sum) {
  // from all completed promotions get creds
  // unblind them and save them
  double promotion_sum = 0.0;
  for (auto& promotion : list) {
    promotion_sum += promotion->approximate_value;
  }

  engine_->Log(FROM_HERE) << "Promotion SUM: " << promotion_sum;

  auto tokens_callback = std::bind(&EmptyBalance::ReportResults, this, _1,
                                   contribution_sum, promotion_sum);

  engine_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {mojom::CredsBatchType::PROMOTION}, tokens_callback);
}

void EmptyBalance::ReportResults(std::vector<mojom::UnblindedTokenPtr> list,
                                 const double contribution_sum,
                                 const double promotion_sum) {
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

  auto url_callback = std::bind(&EmptyBalance::Sent, this, _1);

  promotion_server_.post_bat_loss().Request(total, kVersion, url_callback);
}

void EmptyBalance::Sent(const mojom::Result result) {
  if (result != mojom::Result::OK) {
    return;
  }

  engine_->Log(FROM_HERE) << "Finished empty balance migration";
  engine_->state()->SetEmptyBalanceChecked(true);
}

}  // namespace recovery
}  // namespace brave_rewards::internal
