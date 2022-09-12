/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/credentials/credentials_promotion.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace credential {

CredentialsPromotion::CredentialsPromotion(LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<CredentialsCommon>(ledger)),
    promotion_server_(
        std::make_unique<endpoint::PromotionServer>(ledger)) {
  DCHECK(ledger_ && common_);
}

CredentialsPromotion::~CredentialsPromotion() = default;

void CredentialsPromotion::Start(const CredentialsTrigger& trigger,
                                 ledger::ResultCallback callback) {
  auto get_callback =
      base::BindOnce(&CredentialsPromotion::OnStart, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsPromotion::OnStart(ledger::ResultCallback callback,
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
          base::BindOnce(&CredentialsPromotion::Claim, base::Unretained(this),
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
      auto get_callback =
          base::BindOnce(&CredentialsPromotion::FetchSignedCreds,
                         base::Unretained(this), std::move(callback), trigger);

      ledger_->database()->GetPromotion(
          trigger.id,
          [callback = std::make_shared<decltype(get_callback)>(
               std::move(get_callback))](mojom::PromotionPtr promotion) {
            std::move(*callback).Run(std::move(promotion));
          });
      break;
    }
    case mojom::CredsBatchStatus::SIGNED: {
      auto get_callback =
          base::BindOnce(&CredentialsPromotion::Unblind, base::Unretained(this),
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

void CredentialsPromotion::Blind(ledger::ResultCallback callback,
                                 const CredentialsTrigger& trigger) {
  auto blinded_callback =
      base::BindOnce(&CredentialsPromotion::OnBlind, base::Unretained(this),
                     std::move(callback), trigger);
  common_->GetBlindedCreds(trigger, std::move(blinded_callback));
}

void CredentialsPromotion::OnBlind(ledger::ResultCallback callback,
                                   const CredentialsTrigger& trigger,
                                   mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Blinding failed");
    std::move(callback).Run(result);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsPromotion::Claim, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsPromotion::Claim(ledger::ResultCallback callback,
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
        base::BindOnce(&CredentialsPromotion::RetryPreviousStepSaved,
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
      base::BindOnce(&CredentialsPromotion::OnClaim, base::Unretained(this),
                     std::move(callback), trigger);

  DCHECK(blinded_creds.has_value());
  promotion_server_->post_creds()->Request(
      trigger.id, std::move(blinded_creds.value()), std::move(url_callback));
}

void CredentialsPromotion::OnClaim(ledger::ResultCallback callback,
                                   const CredentialsTrigger& trigger,
                                   mojom::Result result,
                                   const std::string& claim_id) {
  if (result != mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result);
    return;
  }

  auto save_callback =
      base::BindOnce(&CredentialsPromotion::ClaimedSaved,
                     base::Unretained(this), std::move(callback), trigger);

  ledger_->database()->SavePromotionClaimId(
      trigger.id, claim_id,
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsPromotion::ClaimedSaved(ledger::ResultCallback callback,
                                        const CredentialsTrigger& trigger,
                                        mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Claim id was not saved");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback =
      base::BindOnce(&CredentialsPromotion::ClaimStatusSaved,
                     base::Unretained(this), std::move(callback), trigger);

  ledger_->database()->UpdateCredsBatchStatus(
      trigger.id, trigger.type, mojom::CredsBatchStatus::CLAIMED,
      [callback =
           std::make_shared<decltype(save_callback)>(std::move(save_callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
}

void CredentialsPromotion::ClaimStatusSaved(ledger::ResultCallback callback,
                                            const CredentialsTrigger& trigger,
                                            mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Claim status not saved");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsPromotion::FetchSignedCreds,
                     base::Unretained(this), std::move(callback), trigger);

  ledger_->database()->GetPromotion(
      trigger.id,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::PromotionPtr promotion) {
        std::move(*callback).Run(std::move(promotion));
      });
}

void CredentialsPromotion::RetryPreviousStepSaved(
    ledger::ResultCallback callback,
    mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Previous step not saved");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::move(callback).Run(mojom::Result::RETRY);
}

void CredentialsPromotion::FetchSignedCreds(ledger::ResultCallback callback,
                                            const CredentialsTrigger& trigger,
                                            mojom::PromotionPtr promotion) {
  if (!promotion) {
    BLOG(0, "Corrupted data");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  if (promotion->claim_id.empty()) {
    BLOG(0, "Claim id is empty, we will try claim step again");

    auto save_callback =
        base::BindOnce(&CredentialsPromotion::RetryPreviousStepSaved,
                       base::Unretained(this), std::move(callback));

    ledger_->database()->UpdateCredsBatchStatus(
        trigger.id, trigger.type, mojom::CredsBatchStatus::BLINDED,
        [callback = std::make_shared<decltype(save_callback)>(
             std::move(save_callback))](mojom::Result result) {
          std::move(*callback).Run(result);
        });
    return;
  }

  auto url_callback =
      base::BindOnce(&CredentialsPromotion::OnFetchSignedCreds,
                     base::Unretained(this), std::move(callback), trigger);

  promotion_server_->get_signed_creds()->Request(
      trigger.id, promotion->claim_id, std::move(url_callback));
}

void CredentialsPromotion::OnFetchSignedCreds(ledger::ResultCallback callback,
                                              const CredentialsTrigger& trigger,
                                              mojom::Result result,
                                              mojom::CredsBatchPtr batch) {
  // Note: Translate mojom::Result::RETRY_SHORT into
  // mojom::Result::RETRY, as promotion only supports the standard
  // retry
  if (result == mojom::Result::RETRY_SHORT) {
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  if (result != mojom::Result::LEDGER_OK || !batch) {
    BLOG(0, "Problem parsing response");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  batch->trigger_id = trigger.id;
  batch->trigger_type = trigger.type;

  auto save_callback =
      base::BindOnce(&CredentialsPromotion::SignedCredsSaved,
                     base::Unretained(this), std::move(callback), trigger);

  ledger_->database()->SaveSignedCreds(
      std::move(batch), [callback = std::make_shared<decltype(save_callback)>(
                             std::move(save_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

void CredentialsPromotion::SignedCredsSaved(ledger::ResultCallback callback,
                                            const CredentialsTrigger& trigger,
                                            mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Signed creds were not saved");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback =
      base::BindOnce(&CredentialsPromotion::Unblind, base::Unretained(this),
                     std::move(callback), trigger);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id, trigger.type,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::CredsBatchPtr creds_batch) {
        std::move(*callback).Run(std::move(creds_batch));
      });
}

void CredentialsPromotion::Unblind(ledger::ResultCallback callback,
                                   const CredentialsTrigger& trigger,
                                   mojom::CredsBatchPtr creds) {
  if (!creds) {
    BLOG(0, "Corrupted data");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = base::BindOnce(&CredentialsPromotion::VerifyPublicKey,
                                     base::Unretained(this),
                                     std::move(callback), trigger, *creds);

  ledger_->database()->GetPromotion(
      trigger.id,
      [callback = std::make_shared<decltype(get_callback)>(
           std::move(get_callback))](mojom::PromotionPtr promotion) {
        std::move(*callback).Run(std::move(promotion));
      });
}

void CredentialsPromotion::VerifyPublicKey(ledger::ResultCallback callback,
                                           const CredentialsTrigger& trigger,
                                           const mojom::CredsBatch& creds,
                                           mojom::PromotionPtr promotion) {
  if (!promotion) {
    BLOG(0, "Corrupted data");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto promotion_keys = ParseStringToBaseList(promotion->public_keys);

  if (!promotion_keys || promotion_keys->empty()) {
    BLOG(0, "Public key is missing");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  bool valid = false;
  for (auto& item : promotion_keys.value()) {
    if (item.GetString() == creds.public_key) {
      valid = true;
    }
  }

  if (!valid) {
    BLOG(0, "Public key is not valid");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> unblinded_encoded_creds;
  std::string error;
  bool result;
  if (ledger::is_testing) {
    result = UnBlindCredsMock(creds, &unblinded_encoded_creds);
  } else {
    result = UnBlindCreds(creds, &unblinded_encoded_creds, &error);
  }

  if (!result) {
    BLOG(0, "UnBlindTokens: " << error);
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    return;
  }

  const double cred_value =
      promotion->approximate_value / promotion->suggestions;

  auto save_callback =
      base::BindOnce(&CredentialsPromotion::Completed, base::Unretained(this),
                     std::move(callback), trigger);

  uint64_t expires_at = 0ul;
  if (promotion->type != mojom::PromotionType::ADS) {
    expires_at = promotion->expires_at;
  }

  common_->SaveUnblindedCreds(expires_at, cred_value, creds,
                              unblinded_encoded_creds, trigger,
                              std::move(save_callback));
}

void CredentialsPromotion::Completed(ledger::ResultCallback callback,
                                     const CredentialsTrigger& trigger,
                                     mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Unblinded token save failed");
    std::move(callback).Run(result);
    return;
  }

  ledger_->database()->PromotionCredentialCompleted(
      trigger.id,
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          mojom::Result result) { std::move(*callback).Run(result); });
  ledger_->ledger_client()->UnblindedTokensReady();
}

void CredentialsPromotion::RedeemTokens(const CredentialsRedeem& redeem,
                                        ledger::LegacyResultCallback callback) {
  DCHECK(redeem.type != mojom::RewardsType::TRANSFER);

  if (redeem.token_list.empty()) {
    BLOG(0, "Token list empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto& item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto url_callback = std::bind(&CredentialsPromotion::OnRedeemTokens, this, _1,
                                token_id_list, redeem, callback);

  if (redeem.publisher_key.empty()) {
    BLOG(0, "Publisher key is empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  promotion_server_->post_suggestions()->Request(redeem, url_callback);
}

void CredentialsPromotion::OnRedeemTokens(
    mojom::Result result,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse redeem tokens response");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  }

  ledger_->database()->MarkUnblindedTokensAsSpent(token_id_list, redeem.type,
                                                  id, callback);
}

void CredentialsPromotion::DrainTokens(
    const CredentialsRedeem& redeem,
    ledger::PostSuggestionsClaimCallback callback) {
  DCHECK(redeem.type == mojom::RewardsType::TRANSFER);

  if (redeem.token_list.empty()) {
    BLOG(0, "Token list empty");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto& item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto url_callback = base::BindOnce(
      &CredentialsPromotion::OnDrainTokens, base::Unretained(this),
      std::move(callback), std::move(token_id_list), redeem);

  promotion_server_->post_suggestions_claim()->Request(redeem,
                                                       std::move(url_callback));
}

void CredentialsPromotion::OnDrainTokens(
    ledger::PostSuggestionsClaimCallback callback,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    mojom::Result result,
    std::string drain_id) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse drain tokens response");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  }

  DCHECK(redeem.type == mojom::RewardsType::TRANSFER);

  auto mark_tokens_callback = base::BindOnce(
      [](ledger::PostSuggestionsClaimCallback callback, std::string drain_id,
         mojom::Result result) {
        if (result != mojom::Result::LEDGER_OK) {
          BLOG(0, "Failed to mark tokens as spent");
          std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
        } else {
          std::move(callback).Run(mojom::Result::LEDGER_OK,
                                  std::move(drain_id));
        }
      },
      std::move(callback), std::move(drain_id));

  ledger_->database()->MarkUnblindedTokensAsSpent(
      token_id_list, mojom::RewardsType::TRANSFER, id,
      [callback = std::make_shared<decltype(mark_tokens_callback)>(
           std::move(mark_tokens_callback))](mojom::Result result) {
        std::move(*callback).Run(result);
      });
}

}  // namespace credential
}  // namespace ledger
