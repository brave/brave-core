/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/credentials/credentials_sku.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/constants.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

bool IsPublicKeyValid(const std::string& public_key) {
  if (public_key.empty()) {
    return false;
  }

  std::vector<std::string> keys;
  if (ledger::_environment == ledger::type::Environment::PRODUCTION) {
    keys = {
        "yr4w9Y0XZQISBOToATNEl5ADspDUgm7cBSOhfYgPWx4=",  // AC
        "PGLvfpIn8QXuQJEtv2ViQSWw2PppkhexKr1mlvwCpnM="  // User funds
    };
  }

  if (ledger::_environment == ledger::type::Environment::STAGING) {
    keys = {
        "mMMWZrWPlO5b9IB8vF5kUJW4f7ULH1wuEop3NOYqNW0=",  // AC
        "CMezK92X5wmYHVYpr22QhNsTTq6trA/N9Alw+4cKyUY="  // User funds
    };
  }

  if (ledger::_environment == ledger::type::Environment::DEVELOPMENT) {
    keys = {
        "RhfxGp4pT0Kqe2zx4+q+L6lwC3G9v3fIj1L+PbINNzw=",  // AC
        "nsSoWgGMJpIiCGVdYrne03ldQ4zqZOMERVD5eSPhhxc="  // User funds
    };
  }

  auto it = std::find(keys.begin(), keys.end(), public_key);

  return it != keys.end();
}

std::string ConvertItemTypeToString(const std::string& type) {
  int type_int;
  base::StringToInt(type, &type_int);
  switch (static_cast<ledger::type::SKUOrderItemType>(type_int)) {
    case ledger::type::SKUOrderItemType::SINGLE_USE: {
      return "single-use";
    }
    case ledger::type::SKUOrderItemType::NONE: {
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

void CredentialsSKU::Start(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  DCHECK_EQ(trigger.data.size(), 2ul);
  if (trigger.data.empty()) {
    BLOG(0, "Trigger data is missing");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::OnStart,
      this,
      _1,
      trigger,
      callback);

  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id,
      trigger.type,
      get_callback);
}

void CredentialsSKU::OnStart(
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
      auto get_callback = std::bind(&CredentialsSKU::Claim,
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
      FetchSignedCreds(trigger, callback);
      break;
    }
    case type::CredsBatchStatus::SIGNED: {
      auto get_callback = std::bind(&CredentialsSKU::Unblind,
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

void CredentialsSKU::Blind(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto blinded_callback = std::bind(&CredentialsSKU::OnBlind,
      this,
      _1,
      trigger,
      callback);
  common_->GetBlindedCreds(trigger, blinded_callback);
}

void CredentialsSKU::OnBlind(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Claim failed");
    callback(result);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::Claim,
      this,
      _1,
      trigger,
      callback);
  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id,
      trigger.type,
      get_callback);
}

void CredentialsSKU::RetryPreviousStepSaved(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Previous step not saved");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  callback(type::Result::RETRY);
}

void CredentialsSKU::Claim(
    type::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(0, "Creds not found");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(creds->blinded_creds);

  if (!blinded_creds || blinded_creds->GetListDeprecated().empty()) {
    BLOG(0, "Blinded creds are corrupted, we will try to blind again");
    auto save_callback =
        std::bind(&CredentialsSKU::RetryPreviousStepSaved,
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

  auto url_callback = std::bind(&CredentialsSKU::OnClaim,
      this,
      _1,
      trigger,
      callback);


  DCHECK_EQ(trigger.data.size(), 2ul);
  payment_server_->post_credentials()->Request(
      trigger.id,
      trigger.data[0],
      ConvertItemTypeToString(trigger.data[1]),
      std::move(blinded_creds),
      url_callback);
}

void CredentialsSKU::OnClaim(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to claim SKU creds");
    callback(type::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&CredentialsSKU::ClaimStatusSaved,
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

void CredentialsSKU::ClaimStatusSaved(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Claim status not saved: " << result);
    callback(type::Result::RETRY);
    return;
  }

  FetchSignedCreds(trigger, callback);
}

void CredentialsSKU::FetchSignedCreds(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto url_callback = std::bind(&CredentialsSKU::OnFetchSignedCreds,
      this,
      _1,
      _2,
      trigger,
      callback);

  payment_server_->get_credentials()->Request(
      trigger.id,
      trigger.data[0],
      url_callback);
}

void CredentialsSKU::OnFetchSignedCreds(
    const type::Result result,
    type::CredsBatchPtr batch,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't fetch credentials: " << result);
    callback(result);
    return;
  }

  batch->trigger_id = trigger.id;
  batch->trigger_type = trigger.type;

  auto get_callback = std::bind(&CredentialsSKU::SignedCredsSaved,
      this,
      _1,
      trigger,
      callback);
  ledger_->database()->SaveSignedCreds(std::move(batch), get_callback);
}

void CredentialsSKU::SignedCredsSaved(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Signed creds were not saved");
    callback(type::Result::RETRY);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::Unblind,
      this,
      _1,
      trigger,
      callback);
  ledger_->database()->GetCredsBatchByTrigger(
      trigger.id,
      trigger.type,
      get_callback);
}

void CredentialsSKU::Unblind(
    type::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(0, "Corrupted data");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  if (!IsPublicKeyValid(creds->public_key)) {
    BLOG(0, "Public key is not valid");
    callback(type::Result::LEDGER_ERROR);
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
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&CredentialsSKU::Completed,
      this,
      _1,
      trigger,
      callback);

  const uint64_t expires_at = 0ul;

  common_->SaveUnblindedCreds(
      expires_at,
      constant::kVotePrice,
      *creds,
      unblinded_encoded_creds,
      trigger,
      save_callback);
}

void CredentialsSKU::Completed(
    const type::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Unblinded token save failed");
    callback(result);
    return;
  }

  ledger_->ledger_client()->UnblindedTokensReady();
  callback(result);
}

void CredentialsSKU::RedeemTokens(
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  if (redeem.publisher_key.empty() || redeem.token_list.empty()) {
    BLOG(0, "Pub key / token list empty");
    callback(type::Result::LEDGER_ERROR);
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
    const type::Result result,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to submit tokens");
    callback(type::Result::LEDGER_ERROR);
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
