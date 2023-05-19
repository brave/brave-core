/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/credentials/credentials_common.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"

namespace brave_rewards::internal::credential {

void CredentialsCommon::GetBlindedCreds(const CredentialsTrigger& trigger,
                                        ResultCallback callback) {
  const auto creds = GenerateCreds(trigger.size);

  if (creds.empty()) {
    BLOG(0, "Creds are empty");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  const std::string creds_json = GetCredsJSON(creds);
  const auto blinded_creds = GenerateBlindCreds(creds);

  if (blinded_creds.empty()) {
    BLOG(0, "Blinded creds are empty");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  const std::string blinded_creds_json = GetBlindedCredsJSON(blinded_creds);

  auto creds_batch = mojom::CredsBatch::New();
  creds_batch->creds_id = base::GenerateGUID();
  creds_batch->size = trigger.size;
  creds_batch->creds = creds_json;
  creds_batch->blinded_creds = blinded_creds_json;
  creds_batch->trigger_id = trigger.id;
  creds_batch->trigger_type = trigger.type;
  creds_batch->status = mojom::CredsBatchStatus::BLINDED;

  auto save_callback =
      base::BindOnce(&CredentialsCommon::BlindedCredsSaved,
                     base::Unretained(this), std::move(callback));

  ledger().database()->SaveCredsBatch(
      std::move(creds_batch),
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsCommon::BlindedCredsSaved(ResultCallback callback,
                                          mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Creds batch save failed");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK);
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

  ledger().database()->SaveUnblindedTokenList(
      std::move(list), [callback = std::make_shared<decltype(save_callback)>(
                            std::move(save_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

void CredentialsCommon::OnSaveUnblindedCreds(ResultCallback callback,
                                             const CredentialsTrigger& trigger,
                                             mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Token list not saved");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  ledger().database()->UpdateCredsBatchStatus(
      trigger.id, trigger.type, mojom::CredsBatchStatus::FINISHED,
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

}  // namespace brave_rewards::internal::credential
