/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/credentials/credentials_common.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace credential {

CredentialsCommon::CredentialsCommon(RewardsEngineImpl& engine)
    : engine_(engine) {}

CredentialsCommon::~CredentialsCommon() = default;

void CredentialsCommon::GetBlindedCreds(const CredentialsTrigger& trigger,
                                        ResultCallback callback) {
  const auto creds = GenerateCreds(trigger.size);

  if (creds.empty()) {
    engine_->LogError(FROM_HERE) << "Creds are empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  const std::string creds_json = GetCredsJSON(creds);
  const auto blinded_creds = GenerateBlindCreds(creds);

  if (blinded_creds.empty()) {
    engine_->LogError(FROM_HERE) << "Blinded creds are empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  const std::string blinded_creds_json = GetBlindedCredsJSON(blinded_creds);

  auto creds_batch = mojom::CredsBatch::New();
  creds_batch->creds_id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  creds_batch->size = trigger.size;
  creds_batch->creds = creds_json;
  creds_batch->blinded_creds = blinded_creds_json;
  creds_batch->trigger_id = trigger.id;
  creds_batch->trigger_type = trigger.type;
  creds_batch->status = mojom::CredsBatchStatus::BLINDED;

  auto save_callback =
      base::BindOnce(&CredentialsCommon::BlindedCredsSaved,
                     base::Unretained(this), std::move(callback));

  engine_->database()->SaveCredsBatch(
      std::move(creds_batch),
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsCommon::BlindedCredsSaved(ResultCallback callback,
                                          mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Creds batch save failed";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

void CredentialsCommon::SaveUnblindedCreds(
    uint64_t expires_at,
    double token_value,
    const mojom::CredsBatch& creds,
    const std::vector<std::string>& unblinded_encoded_creds,
    const CredentialsTrigger& trigger,
    ResultCallback callback) {
  std::vector<mojom::UnblindedTokenPtr> list;
  mojom::UnblindedTokenPtr unblinded;
  for (auto& cred : unblinded_encoded_creds) {
    unblinded = mojom::UnblindedToken::New();
    unblinded->token_value = cred;
    unblinded->public_key = creds.public_key;
    unblinded->value = token_value;
    unblinded->creds_id = creds.creds_id;
    unblinded->expires_at = expires_at;
    list.push_back(std::move(unblinded));
  }

  auto save_callback =
      base::BindOnce(&CredentialsCommon::OnSaveUnblindedCreds,
                     base::Unretained(this), std::move(callback), trigger);

  engine_->database()->SaveUnblindedTokenList(
      std::move(list), [callback = std::make_shared<decltype(save_callback)>(
                            std::move(save_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

void CredentialsCommon::OnSaveUnblindedCreds(ResultCallback callback,
                                             const CredentialsTrigger& trigger,
                                             mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Token list not saved";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  engine_->database()->UpdateCredsBatchStatus(
      trigger.id, trigger.type, mojom::CredsBatchStatus::FINISHED,
      ToLegacyCallback(std::move(callback)));
}

}  // namespace credential
}  // namespace brave_rewards::internal
