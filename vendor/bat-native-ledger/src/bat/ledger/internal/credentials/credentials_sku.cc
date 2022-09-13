/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/credentials/credentials_sku.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

bool IsPublicKeyValid(const std::string& public_key) {
  if (public_key.empty()) {
    return false;
  }

  std::vector<std::string> keys;
  if (ledger::_environment == ledger::mojom::Environment::PRODUCTION) {
    keys = {
        "yr4w9Y0XZQISBOToATNEl5ADspDUgm7cBSOhfYgPWx4=",  // AC
        "PGLvfpIn8QXuQJEtv2ViQSWw2PppkhexKr1mlvwCpnM="  // User funds
    };
  }

  if (ledger::_environment == ledger::mojom::Environment::STAGING) {
    keys = {
        "mMMWZrWPlO5b9IB8vF5kUJW4f7ULH1wuEop3NOYqNW0=",  // AC
        "CMezK92X5wmYHVYpr22QhNsTTq6trA/N9Alw+4cKyUY="  // User funds
    };
  }

  if (ledger::_environment == ledger::mojom::Environment::DEVELOPMENT) {
    keys = {
        "RhfxGp4pT0Kqe2zx4+q+L6lwC3G9v3fIj1L+PbINNzw=",  // AC
        "nsSoWgGMJpIiCGVdYrne03ldQ4zqZOMERVD5eSPhhxc="  // User funds
    };
  }

  return base::Contains(keys, public_key);
}

std::string ConvertItemTypeToString(const std::string& type) {
  int type_int;
  base::StringToInt(type, &type_int);
  switch (static_cast<ledger::mojom::SKUOrderItemType>(type_int)) {
    case ledger::mojom::SKUOrderItemType::SINGLE_USE: {
      return "single-use";
    }
    case ledger::mojom::SKUOrderItemType::NONE: {
      return "";
    }
  }
}

}  // namespace

namespace ledger {
namespace credential {

CredentialsSKU::CredentialsSKU(LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<CredentialsCommon>(ledger)),
    payment_server_(std::make_unique<endpoint::PaymentServer>(ledger)) {
  DCHECK(ledger_ && common_);
}

CredentialsSKU::~CredentialsSKU() = default;

void CredentialsSKU::Start(const CredentialsTrigger& trigger,
                           ledger::ResultCallback callback) {
  DCHECK_EQ(trigger.data.size(), 2ul);
  if (trigger.data.empty()) {
    BLOG(0, "Trigger data is missing");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::OnStart, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsSKU::OnStart(ledger::ResultCallback callback,
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

      ledger_->database()->GetCredsBatchByTrigger(
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

      ledger_->database()->GetCredsBatchByTrigger(
          trigger.id, trigger.type,
          [callback = std::make_shared<decltype(get_callback)>(
               std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
            std::move(*callback).Run(std::move(creds_batch));
          });
      break;
    }
    case mojom::CredsBatchStatus::FINISHED: {
      std::move(callback).Run(mojom::Result::LEDGER_OK);
      break;
    }
    case mojom::CredsBatchStatus::CORRUPTED: {
      std::move(callback).Run(mojom::Result::LEDGER_ERROR);
      break;
    }
  }
}

void CredentialsSKU::Blind(ledger::ResultCallback callback,
                           const CredentialsTrigger& trigger) {
  auto blinded_callback =
      base::BindOnce(&CredentialsSKU::OnBlind, base::Unretained(this),
                     std::move(callback), trigger);
  common_->GetBlindedCreds(trigger, std::move(blinded_callback));
}

void CredentialsSKU::OnBlind(ledger::ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Claim failed");
    std::move(callback).Run(result);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::Claim, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsSKU::RetryPreviousStepSaved(ledger::ResultCallback callback,
                                            mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Previous step not saved");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::move(callback).Run(mojom::Result::RETRY);
}

void CredentialsSKU::Claim(ledger::ResultCallback callback,
                           const CredentialsTrigger& trigger,
                           mojom::CredsBatchPtr creds) {
  if (!creds) {
    BLOG(0, "Creds not found");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(creds->blinded_creds);

  if (!blinded_creds || blinded_creds->empty()) {
    BLOG(0, "Blinded creds are corrupted, we will try to blind again");
    auto save_callback =
        base::BindOnce(&CredentialsSKU::RetryPreviousStepSaved,
                       base::Unretained(this), std::move(callback));

    ledger_->database()->UpdateCredsBatchStatus(
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
  payment_server_->post_credentials()->Request(
      trigger.id, trigger.data[0], ConvertItemTypeToString(trigger.data[1]),
      std::move(blinded_creds.value()), std::move(url_callback));
}

void CredentialsSKU::OnClaim(ledger::ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to claim SKU creds");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  auto save_callback =
      base::BindOnce(&CredentialsSKU::ClaimStatusSaved, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->UpdateCredsBatchStatus(
      trigger.id, trigger.type, mojom::CredsBatchStatus::CLAIMED,
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsSKU::ClaimStatusSaved(ledger::ResultCallback callback,
                                      const CredentialsTrigger& trigger,
                                      mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Claim status not saved: " << result);
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  FetchSignedCreds(std::move(callback), trigger);
}

void CredentialsSKU::FetchSignedCreds(ledger::ResultCallback callback,
                                      const CredentialsTrigger& trigger) {
  auto url_callback =
      base::BindOnce(&CredentialsSKU::OnFetchSignedCreds,
                     base::Unretained(this), std::move(callback), trigger);

  payment_server_->get_credentials()->Request(trigger.id, trigger.data[0],
                                              std::move(url_callback));
}

void CredentialsSKU::OnFetchSignedCreds(ledger::ResultCallback callback,
                                        const CredentialsTrigger& trigger,
                                        mojom::Result result,
                                        mojom::CredsBatchPtr batch) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't fetch credentials: " << result);
    std::move(callback).Run(result);
    return;
  }

  batch->trigger_id = trigger.id;
  batch->trigger_type = trigger.type;

  auto get_callback =
      base::BindOnce(&CredentialsSKU::SignedCredsSaved, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->SaveSignedCreds(
      std::move(batch), [callback = std::make_shared<decltype(get_callback)>(
                             std::move(get_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

void CredentialsSKU::SignedCredsSaved(ledger::ResultCallback callback,
                                      const CredentialsTrigger& trigger,
                                      mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Signed creds were not saved");
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsSKU::Unblind, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsSKU::Unblind(ledger::ResultCallback callback,
                             const CredentialsTrigger& trigger,
                             mojom::CredsBatchPtr creds) {
  if (!creds) {
    BLOG(0, "Corrupted data");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  if (!IsPublicKeyValid(creds->public_key)) {
    BLOG(0, "Public key is not valid");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> unblinded_encoded_creds;
  std::string error;
  bool result;
  if (ledger::is_testing) {
    result = UnBlindCredsMock(*creds, &unblinded_encoded_creds);
  } else {
    result = UnBlindCreds(*creds, &unblinded_encoded_creds, &error);
  }

  if (!result) {
    BLOG(0, "UnBlindTokens: " << error);
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback =
      base::BindOnce(&CredentialsSKU::Completed, base::Unretained(this),
                     std::move(callback), trigger);

  const uint64_t expires_at = 0ul;

  common_->SaveUnblindedCreds(expires_at, constant::kVotePrice, *creds,
                              unblinded_encoded_creds, trigger,
                              std::move(save_callback));
}

void CredentialsSKU::Completed(ledger::ResultCallback callback,
                               const CredentialsTrigger& trigger,
                               mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Unblinded token save failed");
    std::move(callback).Run(result);
    return;
  }

  ledger_->ledger_client()->UnblindedTokensReady();
  std::move(callback).Run(result);
}

void CredentialsSKU::RedeemTokens(const CredentialsRedeem& redeem,
                                  ledger::LegacyResultCallback callback) {
  if (redeem.publisher_key.empty() || redeem.token_list.empty()) {
    BLOG(0, "Pub key / token list empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto & item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto url_callback = std::bind(&CredentialsSKU::OnRedeemTokens,
      this,
      _1,
      token_id_list,
      redeem,
      callback);

  payment_server_->post_votes()->Request(redeem, url_callback);
}

void CredentialsSKU::OnRedeemTokens(
    mojom::Result result,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to submit tokens");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  } else if (!redeem.order_id.empty()) {
    id = redeem.order_id;
  }

  ledger_->database()->MarkUnblindedTokensAsSpent(
      token_id_list,
      redeem.type,
      id,
      callback);
}

}  // namespace credential
}  // namespace ledger
