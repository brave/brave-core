/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/credentials/credentials_sku.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

namespace {

std::string ConvertItemTypeToString(const std::string& type) {
  int type_int;
  base::StringToInt(type, &type_int);
  switch (static_cast<mojom::SKUOrderItemType>(type_int)) {
    case mojom::SKUOrderItemType::SINGLE_USE: {
      return "single-use";
    }
    case mojom::SKUOrderItemType::NONE: {
      return "";
    }
  }
}

}  // namespace

namespace credential {

CredentialsSKU::CredentialsSKU(RewardsEngineImpl& engine)
    : engine_(engine), common_(engine), payment_server_(engine) {}

CredentialsSKU::~CredentialsSKU() = default;

void CredentialsSKU::Start(const CredentialsTrigger& trigger,
                           ResultCallback callback) {
  DCHECK_EQ(trigger.data.size(), 2ul);
  if (trigger.data.empty()) {
    engine_->LogError(FROM_HERE) << "Trigger data is missing";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::OnStart, weak_factory_.GetWeakPtr(),
                     std::move(callback), trigger);

  engine_->database()->GetCredsBatchByTrigger(trigger.id, trigger.type,
                                              std::move(get_callback));
}

void CredentialsSKU::OnStart(ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::CredsBatchPtr creds) {
  mojom::CredsBatchStatus status = mojom::CredsBatchStatus::NONE;
  if (creds) {
    status = creds->status;
  }

  switch (status) {
    case mojom::CredsBatchStatus::NONE: {
      Blind(std::move(callback), trigger);
      break;
    }
    case mojom::CredsBatchStatus::BLINDED: {
      auto get_callback =
          base::BindOnce(&CredentialsSKU::Claim, weak_factory_.GetWeakPtr(),
                         std::move(callback), trigger);

      engine_->database()->GetCredsBatchByTrigger(trigger.id, trigger.type,
                                                  std::move(get_callback));
      break;
    }
    case mojom::CredsBatchStatus::CLAIMED: {
      FetchSignedCreds(std::move(callback), trigger);
      break;
    }
    case mojom::CredsBatchStatus::SIGNED: {
      auto get_callback =
          base::BindOnce(&CredentialsSKU::Unblind, weak_factory_.GetWeakPtr(),
                         std::move(callback), trigger);

      engine_->database()->GetCredsBatchByTrigger(trigger.id, trigger.type,
                                                  std::move(get_callback));
      break;
    }
    case mojom::CredsBatchStatus::FINISHED: {
      std::move(callback).Run(mojom::Result::OK);
      break;
    }
    case mojom::CredsBatchStatus::CORRUPTED: {
      std::move(callback).Run(mojom::Result::FAILED);
      break;
    }
  }
}

void CredentialsSKU::Blind(ResultCallback callback,
                           const CredentialsTrigger& trigger) {
  auto blinded_callback =
      base::BindOnce(&CredentialsSKU::OnBlind, weak_factory_.GetWeakPtr(),
                     std::move(callback), trigger);
  common_.GetBlindedCreds(trigger, std::move(blinded_callback));
}

void CredentialsSKU::OnBlind(ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Claim failed";
    std::move(callback).Run(result);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::Claim, weak_factory_.GetWeakPtr(),
                     std::move(callback), trigger);

  engine_->database()->GetCredsBatchByTrigger(trigger.id, trigger.type,
                                              std::move(get_callback));
}

void CredentialsSKU::RetryPreviousStepSaved(ResultCallback callback,
                                            mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Previous step not saved";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::move(callback).Run(mojom::Result::RETRY);
}

void CredentialsSKU::Claim(ResultCallback callback,
                           const CredentialsTrigger& trigger,
                           mojom::CredsBatchPtr creds) {
  if (!creds) {
    engine_->LogError(FROM_HERE) << "Creds not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(creds->blinded_creds);

  if (!blinded_creds || blinded_creds->empty()) {
    engine_->LogError(FROM_HERE)
        << "Blinded creds are corrupted, we will try to blind again";
    auto save_callback =
        base::BindOnce(&CredentialsSKU::RetryPreviousStepSaved,
                       weak_factory_.GetWeakPtr(), std::move(callback));

    engine_->database()->UpdateCredsBatchStatus(trigger.id, trigger.type,
                                                mojom::CredsBatchStatus::NONE,
                                                std::move(save_callback));
    return;
  }

  auto url_callback =
      base::BindOnce(&CredentialsSKU::OnClaim, weak_factory_.GetWeakPtr(),
                     std::move(callback), trigger);

  DCHECK_EQ(trigger.data.size(), 2ul);
  DCHECK(blinded_creds.has_value());
  payment_server_.post_credentials().Request(
      trigger.id, trigger.data[0], ConvertItemTypeToString(trigger.data[1]),
      std::move(blinded_creds.value()), std::move(url_callback));
}

void CredentialsSKU::OnClaim(ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to claim SKU creds";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  auto save_callback =
      base::BindOnce(&CredentialsSKU::ClaimStatusSaved,
                     weak_factory_.GetWeakPtr(), std::move(callback), trigger);

  engine_->database()->UpdateCredsBatchStatus(trigger.id, trigger.type,
                                              mojom::CredsBatchStatus::CLAIMED,
                                              std::move(save_callback));
}

void CredentialsSKU::ClaimStatusSaved(ResultCallback callback,
                                      const CredentialsTrigger& trigger,
                                      mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Claim status not saved: " << result;
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  FetchSignedCreds(std::move(callback), trigger);
}

void CredentialsSKU::FetchSignedCreds(ResultCallback callback,
                                      const CredentialsTrigger& trigger) {
  auto url_callback =
      base::BindOnce(&CredentialsSKU::OnFetchSignedCreds,
                     weak_factory_.GetWeakPtr(), std::move(callback), trigger);

  payment_server_.get_credentials().Request(trigger.id, trigger.data[0],
                                            std::move(url_callback));
}

void CredentialsSKU::OnFetchSignedCreds(ResultCallback callback,
                                        const CredentialsTrigger& trigger,
                                        mojom::Result result,
                                        mojom::CredsBatchPtr batch) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Couldn't fetch credentials: " << result;
    std::move(callback).Run(result);
    return;
  }

  batch->trigger_id = trigger.id;
  batch->trigger_type = trigger.type;

  auto get_callback =
      base::BindOnce(&CredentialsSKU::SignedCredsSaved,
                     weak_factory_.GetWeakPtr(), std::move(callback), trigger);

  engine_->database()->SaveSignedCreds(std::move(batch),
                                       std::move(get_callback));
}

void CredentialsSKU::SignedCredsSaved(ResultCallback callback,
                                      const CredentialsTrigger& trigger,
                                      mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Signed creds were not saved";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::Unblind, weak_factory_.GetWeakPtr(),
                     std::move(callback), trigger);

  engine_->database()->GetCredsBatchByTrigger(trigger.id, trigger.type,
                                              std::move(get_callback));
}

void CredentialsSKU::Unblind(ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::CredsBatchPtr creds) {
  if (!creds) {
    engine_->LogError(FROM_HERE) << "Corrupted data";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::vector valid_public_keys = {
      engine_->Get<EnvironmentConfig>().auto_contribute_public_key(),
      engine_->Get<EnvironmentConfig>().user_funds_public_key()};

  if (!base::Contains(valid_public_keys, creds->public_key)) {
    engine_->LogError(FROM_HERE) << "Public key is not valid";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::vector<std::string> unblinded_encoded_creds;
  if (engine_->options().is_testing) {
    unblinded_encoded_creds = UnBlindCredsMock(*creds);
  } else {
    auto result = UnBlindCreds(*creds);
    if (!result.has_value()) {
      engine_->LogError(FROM_HERE) << "UnBlindTokens error";
      engine_->Log(FROM_HERE) << result.error();
      std::move(callback).Run(mojom::Result::FAILED);
      return;
    }
    unblinded_encoded_creds = std::move(result).value();
  }

  auto save_callback =
      base::BindOnce(&CredentialsSKU::Completed, weak_factory_.GetWeakPtr(),
                     std::move(callback), trigger);

  const uint64_t expires_at = 0ul;

  common_.SaveUnblindedCreds(expires_at, constant::kVotePrice, *creds,
                             unblinded_encoded_creds, trigger,
                             std::move(save_callback));
}

void CredentialsSKU::Completed(ResultCallback callback,
                               const CredentialsTrigger& trigger,
                               mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Unblinded token save failed";
    std::move(callback).Run(result);
    return;
  }

  std::move(callback).Run(result);
}

void CredentialsSKU::RedeemTokens(const CredentialsRedeem& redeem,
                                  ResultCallback callback) {
  if (redeem.publisher_key.empty() || redeem.token_list.empty()) {
    engine_->LogError(FROM_HERE) << "Pub key / token list empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto& item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  payment_server_.post_votes().Request(
      redeem, base::BindOnce(
                  &CredentialsSKU::OnRedeemTokens, weak_factory_.GetWeakPtr(),
                  std::move(token_id_list), redeem, std::move(callback)));
}

void CredentialsSKU::OnRedeemTokens(std::vector<std::string> token_id_list,
                                    CredentialsRedeem redeem,
                                    ResultCallback callback,
                                    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to submit tokens";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  } else if (!redeem.order_id.empty()) {
    id = redeem.order_id;
  }

  engine_->database()->MarkUnblindedTokensAsSpent(token_id_list, redeem.type,
                                                  id, std::move(callback));
}

}  // namespace credential
}  // namespace brave_rewards::internal
