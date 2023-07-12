/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/promotion/promotion_transfer.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace promotion {

PromotionTransfer::PromotionTransfer(RewardsEngineImpl& engine)
    : engine_(engine), credentials_(engine) {}

PromotionTransfer::~PromotionTransfer() = default;

void PromotionTransfer::Start(PostSuggestionsClaimCallback callback) {
  auto tokens_callback =
      base::BindOnce(&PromotionTransfer::OnGetSpendableUnblindedTokens,
                     base::Unretained(this), std::move(callback));

  engine_->database()->GetSpendableUnblindedTokens(
      [callback = std::make_shared<decltype(tokens_callback)>(std::move(
           tokens_callback))](std::vector<mojom::UnblindedTokenPtr> tokens) {
        std::move(*callback).Run(std::move(tokens));
      });
}

void PromotionTransfer::OnGetSpendableUnblindedTokens(
    PostSuggestionsClaimCallback callback,
    std::vector<mojom::UnblindedTokenPtr> tokens) {
  std::vector<mojom::UnblindedToken> token_list;
  for (auto& token : tokens) {
    token_list.push_back(*token);
  }

  if (token_list.empty()) {
    return std::move(callback).Run(mojom::Result::OK, "");
  }

  credential::CredentialsRedeem redeem;
  redeem.type = mojom::RewardsType::TRANSFER;
  redeem.processor = mojom::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = std::move(token_list);

  credentials_.DrainTokens(
      redeem, base::BindOnce(&PromotionTransfer::OnDrainTokens,
                             base::Unretained(this), std::move(callback),
                             redeem.token_list.size() * constant::kVotePrice));
}

void PromotionTransfer::OnDrainTokens(PostSuggestionsClaimCallback callback,
                                      double transfer_amount,
                                      mojom::Result result,
                                      std::string drain_id) const {
  if (result == mojom::Result::OK) {
    engine_->database()->SaveEventLog(log::kPromotionVBATDrained,
                                      base::NumberToString(transfer_amount));
  }

  std::move(callback).Run(result, std::move(drain_id));
}

}  // namespace promotion
}  // namespace brave_rewards::internal
