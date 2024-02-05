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

using std::placeholders::_1;

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
    BLOG(0, "Trigger data is missing");
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::OnStart, base::Unretained(this),
                     std::move(callback), trigger);

  engine_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
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
          base::BindOnce(&CredentialsSKU::Claim, base::Unretained(this),
                         std::move(callback), trigger);

      engine_->database()->GetCredsBatchByTrigger(
          trigger.id, trigger.type,
          [callback = std::make_shared<decltype(get_callback)>(
               std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
            std::move(*callback).Run(std::move(creds_batch));
          });
      break;
    }
    case mojom::CredsBatchStatus::CLAIMED: {
      FetchSignedCreds(std::move(callback), trigger);
      break;
    }
    case mojom::CredsBatchStatus::SIGNED: {
      auto get_callback =
          base::BindOnce(&CredentialsSKU::Unblind, base::Unretained(this),
                         std::move(callback), trigger);

      engine_->database()->GetCredsBatchByTrigger(
          trigger.id, trigger.type,
          [callback = std::make_shared<decltype(get_callback)>(
               std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
            std::move(*callback).Run(std::move(creds_batch));
          });
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
      base::BindOnce(&CredentialsSKU::OnBlind, base::Unretained(this),
                     std::move(callback), trigger);
  common_.GetBlindedCreds(trigger, std::move(blinded_callback));
}

void CredentialsSKU::OnBlind(ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::Result result) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Claim failed");
    std::move(callback).Run(result);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::Claim, base::Unretained(this),
                     std::move(callback), trigger);

  engine_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsSKU::RetryPreviousStepSaved(ResultCallback callback,
                                            mojom::Result result) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Previous step not saved");
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::move(callback).Run(mojom::Result::RETRY);
}

void CredentialsSKU::Claim(ResultCallback callback,
                           const CredentialsTrigger& trigger,
                           mojom::CredsBatchPtr creds) {
  if (!creds) {
    BLOG(0, "Creds not found");
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(creds->blinded_creds);

  if (!blinded_creds || blinded_creds->empty()) {
    BLOG(0, "Blinded creds are corrupted, we will try to blind again");
    auto save_callback =
        base::BindOnce(&CredentialsSKU::RetryPreviousStepSaved,
                       base::Unretained(this), std::move(callback));

    engine_->database()->UpdateCredsBatchStatus(
        trigger.id, trigger.type, mojom::CredsBatchStatus::NONE,
        [callback = std::make_shared<decltype(save_callback)>(
             std::move(save_callback))](mojom::Result result) {
          std::move(*callback).Run(result);
        });
    return;
  }

  auto url_callback =
      base::BindOnce(&CredentialsSKU::OnClaim, base::Unretained(this),
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
    BLOG(0, "Failed to claim SKU creds");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  auto save_callback =
      base::BindOnce(&CredentialsSKU::ClaimStatusSaved, base::Unretained(this),
                     std::move(callback), trigger);

  engine_->database()->UpdateCredsBatchStatus(
      trigger.id, trigger.type, mojom::CredsBatchStatus::CLAIMED,
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsSKU::ClaimStatusSaved(ResultCallback callback,
                                      const CredentialsTrigger& trigger,
                                      mojom::Result result) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Claim status not saved: " << result);
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  FetchSignedCreds(std::move(callback), trigger);
}

void CredentialsSKU::FetchSignedCreds(ResultCallback callback,
                                      const CredentialsTrigger& trigger) {
  auto url_callback =
      base::BindOnce(&CredentialsSKU::OnFetchSignedCreds,
                     base::Unretained(this), std::move(callback), trigger);

  payment_server_.get_credentials().Request(trigger.id, trigger.data[0],
                                            std::move(url_callback));
}

void CredentialsSKU::OnFetchSignedCreds(ResultCallback callback,
                                        const CredentialsTrigger& trigger,
                                        mojom::Result result,
                                        mojom::CredsBatchPtr batch) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Couldn't fetch credentials: " << result);
    std::move(callback).Run(result);
    return;
  }

  batch->trigger_id = trigger.id;
  batch->trigger_type = trigger.type;

  auto get_callback =
      base::BindOnce(&CredentialsSKU::SignedCredsSaved, base::Unretained(this),
                     std::move(callback), trigger);

  engine_->database()->SaveSignedCreds(
      std::move(batch), [callback = std::make_shared<decltype(get_callback)>(
                             std::move(get_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

void CredentialsSKU::SignedCredsSaved(ResultCallback callback,
                                      const CredentialsTrigger& trigger,
                                      mojom::Result result) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Signed creds were not saved");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::Unblind, base::Unretained(this),
                     std::move(callback), trigger);

  engine_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsSKU::Unblind(ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::CredsBatchPtr creds) {
  if (!creds) {
    BLOG(0, "Corrupted data");
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::vector valid_public_keys = {
      engine_->Get<EnvironmentConfig>().auto_contribute_public_key(),
      engine_->Get<EnvironmentConfig>().user_funds_public_key()};

  if (!base::Contains(valid_public_keys, creds->public_key)) {
    BLOG(0, "Public key is not valid");
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::vector<std::string> unblinded_encoded_creds;
  if (is_testing) {
    unblinded_encoded_creds = UnBlindCredsMock(*creds);
  } else {
    auto result = UnBlindCreds(*creds);
    if (!result.has_value()) {
      BLOG(0, "UnBlindTokens: " << result.error());
      std::move(callback).Run(mojom::Result::FAILED);
      return;
    }
    unblinded_encoded_creds = std::move(result).value();
  }

  auto save_callback =
      base::BindOnce(&CredentialsSKU::Completed, base::Unretained(this),
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
    BLOG(0, "Unblinded token save failed");
    std::move(callback).Run(result);
    return;
  }

  engine_->client()->UnblindedTokensReady();
  std::move(callback).Run(result);
}

void CredentialsSKU::RedeemTokens(const CredentialsRedeem& redeem,
                                  LegacyResultCallback callback) {
  if (redeem.publisher_key.empty() || redeem.token_list.empty()) {
    BLOG(0, "Pub key / token list empty");
    callback(mojom::Result::FAILED);
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto& item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto url_callback = std::bind(&CredentialsSKU::OnRedeemTokens, this, _1,
                                token_id_list, redeem, callback);

  payment_server_.post_votes().Request(redeem, url_callback);
}

void CredentialsSKU::OnRedeemTokens(
    mojom::Result result,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Failed to submit tokens");
    callback(mojom::Result::FAILED);
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  } else if (!redeem.order_id.empty()) {
    id = redeem.order_id;
  }

  engine_->database()->MarkUnblindedTokensAsSpent(token_id_list, redeem.type,
                                                  id, callback);
}

}  // namespace credential
}  // namespace brave_rewards::internal
