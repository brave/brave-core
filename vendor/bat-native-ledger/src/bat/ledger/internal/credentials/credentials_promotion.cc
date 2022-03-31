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
using std::placeholders::_2;
using std::placeholders::_3;

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

void CredentialsPromotion::Start(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto get_callback = std::bind(&CredentialsPromotion::OnStart,
          this,
          _1,
          trigger,
          callback);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id,
      trigger.type,
      get_callback);
}

void CredentialsPromotion::OnStart(
    type::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  type::CredsBatchStatus status = type::CredsBatchStatus::NONE;
  if (creds) {
    status = creds->status;
  }

  switch (status) {
    case type::CredsBatchStatus::NONE: {
      Blind(trigger, callback);
      break;
    }
    case type::CredsBatchStatus::BLINDED: {
      auto get_callback = std::bind(&CredentialsPromotion::Claim,
          this,
          _1,
          trigger,
          callback);
      ledger_->database()->GetCredsBatchByTrigger(
          trigger.id,
          trigger.type,
          get_callback);
      break;
    }
    case type::CredsBatchStatus::CLAIMED: {
      auto get_callback = std::bind(&CredentialsPromotion::FetchSignedCreds,
          this,
          _1,
          trigger,
          callback);
      ledger_->database()->GetPromotion(trigger.id, get_callback);
      break;
    }
    case type::CredsBatchStatus::SIGNED: {
      auto get_callback = std::bind(&CredentialsPromotion::Unblind,
          this,
          _1,
          trigger,
          callback);
      ledger_->database()->GetCredsBatchByTrigger(
          trigger.id,
          trigger.type,
          get_callback);
      break;
    }
    case type::CredsBatchStatus::FINISHED: {
      callback(type::Result::LEDGER_OK);
      break;
    }
    case type::CredsBatchStatus::CORRUPTED: {
      callback(type::Result::LEDGER_ERROR);
      break;
    }
  }
}

void CredentialsPromotion::Blind(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto blinded_callback = std::bind(&CredentialsPromotion::OnBlind,
      this,
      _1,
      trigger,
      callback);
  common_->GetBlindedCreds(trigger, blinded_callback);
}

void CredentialsPromotion::OnBlind(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Blinding failed");
    callback(result);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::Claim,
      this,
      _1,
      trigger,
      callback);
  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id,
      trigger.type,
      get_callback);
}

void CredentialsPromotion::Claim(
    type::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(0, "Creds not found");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(creds->blinded_creds);

  if (!blinded_creds || blinded_creds->empty()) {
    BLOG(0, "Blinded creds are corrupted, we will try to blind again");
    auto save_callback =
        std::bind(&CredentialsPromotion::RetryPreviousStepSaved,
            this,
            _1,
            callback);

    ledger_->database()->UpdateCredsBatchStatus(
        trigger.id,
        trigger.type,
        type::CredsBatchStatus::NONE,
        save_callback);
    return;
  }

  auto url_callback = std::bind(&CredentialsPromotion::OnClaim,
      this,
      _1,
      _2,
      trigger,
      callback);

  DCHECK(blinded_creds.has_value());
  promotion_server_->post_creds()->Request(
      trigger.id, std::move(blinded_creds.value()), url_callback);
}

void CredentialsPromotion::OnClaim(
    const type::Result result,
    const std::string& claim_id,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  auto save_callback = std::bind(&CredentialsPromotion::ClaimedSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->database()->SavePromotionClaimId(
      trigger.id,
      claim_id,
      save_callback);
}

void CredentialsPromotion::ClaimedSaved(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Claim id was not saved");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&CredentialsPromotion::ClaimStatusSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->database()->UpdateCredsBatchStatus(
      trigger.id,
      trigger.type,
      type::CredsBatchStatus::CLAIMED,
      save_callback);
}

void CredentialsPromotion::ClaimStatusSaved(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Claim status not saved");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::FetchSignedCreds,
      this,
      _1,
      trigger,
      callback);
  ledger_->database()->GetPromotion(trigger.id, get_callback);
}

void CredentialsPromotion::RetryPreviousStepSaved(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Previous step not saved");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  callback(type::Result::RETRY);
}

void CredentialsPromotion::FetchSignedCreds(
    type::PromotionPtr promotion,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!promotion) {
    BLOG(0, "Corrupted data");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  if (promotion->claim_id.empty()) {
    BLOG(0, "Claim id is empty, we will try claim step again");

    auto save_callback =
        std::bind(&CredentialsPromotion::RetryPreviousStepSaved,
            this,
            _1,
            callback);

    ledger_->database()->UpdateCredsBatchStatus(
        trigger.id,
        trigger.type,
        type::CredsBatchStatus::BLINDED,
        save_callback);
    return;
  }

  auto url_callback = std::bind(&CredentialsPromotion::OnFetchSignedCreds,
      this,
      _1,
      _2,
      trigger,
      callback);

  promotion_server_->get_signed_creds()->Request(
      trigger.id,
      promotion->claim_id,
      url_callback);
}

void CredentialsPromotion::OnFetchSignedCreds(
    const type::Result result,
    type::CredsBatchPtr batch,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  // Note: Translate type::Result::RETRY_SHORT into
  // type::Result::RETRY, as promotion only supports the standard
  // retry
  if (result == type::Result::RETRY_SHORT) {
    callback(type::Result::RETRY);
    return;
  }

  if (result != type::Result::LEDGER_OK || !batch) {
    BLOG(0, "Problem parsing response");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  batch->trigger_id = trigger.id;
  batch->trigger_type = trigger.type;

  auto save_callback = std::bind(&CredentialsPromotion::SignedCredsSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->database()->SaveSignedCreds(std::move(batch), save_callback);
}

void CredentialsPromotion::SignedCredsSaved(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Signed creds were not saved");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::Unblind,
      this,
      _1,
      trigger,
      callback);
  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id,
      trigger.type,
      get_callback);
}

void CredentialsPromotion::Unblind(
    type::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(0, "Corrupted data");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::VerifyPublicKey,
      this,
      _1,
      trigger,
      *creds,
      callback);
  ledger_->database()->GetPromotion(trigger.id, get_callback);
}

void CredentialsPromotion::VerifyPublicKey(
    type::PromotionPtr promotion,
    const CredentialsTrigger& trigger,
    const type::CredsBatch& creds,
    ledger::ResultCallback callback) {
  if (!promotion) {
    BLOG(0, "Corrupted data");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto promotion_keys = ParseStringToBaseList(promotion->public_keys);

  if (!promotion_keys || promotion_keys->empty()) {
    BLOG(0, "Public key is missing");
    callback(type::Result::LEDGER_ERROR);
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
    callback(type::Result::LEDGER_ERROR);
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
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const double cred_value =
      promotion->approximate_value / promotion->suggestions;

  auto save_callback = std::bind(&CredentialsPromotion::Completed,
      this,
      _1,
      trigger,
      callback);

  uint64_t expires_at = 0ul;
  if (promotion->type != type::PromotionType::ADS) {
    expires_at = promotion->expires_at;
  }

  common_->SaveUnblindedCreds(
      expires_at,
      cred_value,
      creds,
      unblinded_encoded_creds,
      trigger,
      save_callback);
}

void CredentialsPromotion::Completed(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Unblinded token save failed");
    callback(result);
    return;
  }

  ledger_->database()->PromotionCredentialCompleted(trigger.id, callback);
  ledger_->ledger_client()->UnblindedTokensReady();
}

void CredentialsPromotion::RedeemTokens(
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  DCHECK(redeem.type != type::RewardsType::TRANSFER);

  if (redeem.token_list.empty()) {
    BLOG(0, "Token list empty");
    callback(type::Result::LEDGER_ERROR);
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
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  promotion_server_->post_suggestions()->Request(redeem, url_callback);
}

void CredentialsPromotion::OnRedeemTokens(
    const type::Result result,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse redeem tokens response");
    callback(type::Result::LEDGER_ERROR);
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
  DCHECK(redeem.type == type::RewardsType::TRANSFER);

  if (redeem.token_list.empty()) {
    BLOG(0, "Token list empty");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto& item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto url_callback = std::bind(&CredentialsPromotion::OnDrainTokens, this, _1,
                                _2, token_id_list, redeem, callback);

  promotion_server_->post_suggestions_claim()->Request(redeem, url_callback);
}

void CredentialsPromotion::OnDrainTokens(
    const type::Result result,
    std::string drain_id,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    ledger::PostSuggestionsClaimCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse drain tokens response");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  }

  DCHECK(redeem.type == type::RewardsType::TRANSFER);
  ledger_->database()->MarkUnblindedTokensAsSpent(
      token_id_list, type::RewardsType::TRANSFER, id,
      [drain_id, callback](const type::Result result) {
        if (result != type::Result::LEDGER_OK) {
          BLOG(0, "Failed to mark tokens as spent");
          callback(type::Result::LEDGER_ERROR, "");
        } else {
          callback(type::Result::LEDGER_OK, drain_id);
        }
      });
}

}  // namespace credential
}  // namespace ledger
