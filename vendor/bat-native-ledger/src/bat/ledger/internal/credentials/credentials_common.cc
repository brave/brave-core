/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/credentials/credentials_common.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace credential {

CredentialsCommon::CredentialsCommon(LedgerImpl *ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

CredentialsCommon::~CredentialsCommon() = default;

void CredentialsCommon::GetBlindedCreds(const CredentialsTrigger& trigger,
                                        ledger::ResultCallback callback) {
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

  ledger_->database()->SaveCredsBatch(
      std::move(creds_batch),
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsCommon::BlindedCredsSaved(ledger::ResultCallback callback,
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
    ledger::ResultCallback callback) {
  std::vector<mojom::UnblindedTokenPtr> list;
  mojom::UnblindedTokenPtr unblinded;
  for (auto & cred : unblinded_encoded_creds) {
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

  ledger_->database()->SaveUnblindedTokenList(
      std::move(list), [callback = std::make_shared<decltype(save_callback)>(
                            std::move(save_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

void CredentialsCommon::OnSaveUnblindedCreds(ledger::ResultCallback callback,
                                             const CredentialsTrigger& trigger,
                                             mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Token list not saved");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  ledger_->database()->UpdateCredsBatchStatus(
      trigger.id, trigger.type, mojom::CredsBatchStatus::FINISHED,
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

}  // namespace credential
}  // namespace ledger
