/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/credentials/credentials_common.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/response/response_credentials.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_credentials {

CredentialsCommon::CredentialsCommon(bat_ledger::LedgerImpl *ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

CredentialsCommon::~CredentialsCommon() = default;

void CredentialsCommon::GetBlindedCreds(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  const auto creds = GenerateCreds(trigger.size);

  if (creds.empty()) {
    BLOG(0, "Creds are empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string creds_json = GetCredsJSON(creds);
  const auto blinded_creds = GenerateBlindCreds(creds);

  if (blinded_creds.empty()) {
    BLOG(0, "Blinded creds are empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string blinded_creds_json = GetBlindedCredsJSON(blinded_creds);

  auto creds_batch = ledger::CredsBatch::New();
  creds_batch->creds_id = base::GenerateGUID();
  creds_batch->size = trigger.size;
  creds_batch->creds = creds_json;
  creds_batch->blinded_creds = blinded_creds_json;
  creds_batch->trigger_id = trigger.id;
  creds_batch->trigger_type = trigger.type;
  creds_batch->status = ledger::CredsBatchStatus::BLINDED;

  auto save_callback = std::bind(&CredentialsCommon::BlindedCredsSaved,
      this,
      _1,
      callback);

  ledger_->database()->SaveCredsBatch(std::move(creds_batch), save_callback);
}

void CredentialsCommon::BlindedCredsSaved(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Creds batch save failed");
    callback(ledger::Result::RETRY);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void CredentialsCommon::GetSignedCredsFromResponse(
    const CredentialsTrigger& trigger,
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  base::Value parsed_response(base::Value::Type::DICTIONARY);
  const ledger::Result result =
      braveledger_response_util::ParseSignedCreds(response, &parsed_response);

  if (result == ledger::Result::RETRY_SHORT) {
    callback(ledger::Result::RETRY_SHORT);
    return;
  }

  if (result != ledger::Result::LEDGER_OK || parsed_response.DictSize() != 3) {
    BLOG(0, "Parsing failed");
    callback(ledger::Result::RETRY);
    return;
  }

  auto* signed_creds = parsed_response.FindListKey("signed_creds");
  if (!signed_creds || signed_creds->GetList().empty()) {
    BLOG(0, "Failed to parse signed creds");
    callback(ledger::Result::RETRY);
    return;
  }

  auto* public_key = parsed_response.FindStringKey("public_key");
  if (!public_key || public_key->empty()) {
    BLOG(0, "Public key is empty");
    callback(ledger::Result::RETRY);
    return;
  }

  auto* batch_proof = parsed_response.FindStringKey("batch_proof");
  if (!batch_proof || batch_proof->empty()) {
    BLOG(0, "Batch proof is empty");
    callback(ledger::Result::RETRY);
    return;
  }

  auto creds_batch = ledger::CredsBatch::New();
  creds_batch->trigger_id = trigger.id;
  creds_batch->trigger_type = trigger.type;
  base::JSONWriter::Write(*signed_creds, &creds_batch->signed_creds);
  creds_batch->public_key = *public_key;
  creds_batch->batch_proof = *batch_proof;

  ledger_->database()->SaveSignedCreds(std::move(creds_batch), callback);
}

void CredentialsCommon::SaveUnblindedCreds(
    const uint64_t expires_at,
    const double token_value,
    const ledger::CredsBatch& creds,
    const std::vector<std::string>& unblinded_encoded_creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger::UnblindedTokenList list;
  ledger::UnblindedTokenPtr unblinded;
  for (auto & cred : unblinded_encoded_creds) {
    unblinded = ledger::UnblindedToken::New();
    unblinded->token_value = cred;
    unblinded->public_key = creds.public_key;
    unblinded->value = token_value;
    unblinded->creds_id = creds.creds_id;
    unblinded->expires_at = expires_at;
    list.push_back(std::move(unblinded));
  }

  auto save_callback = std::bind(&CredentialsCommon::OnSaveUnblindedCreds,
      this,
      _1,
      trigger,
      callback);

  ledger_->database()->SaveUnblindedTokenList(std::move(list), save_callback);
}

void CredentialsCommon::OnSaveUnblindedCreds(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Token list not saved");
    callback(ledger::Result::RETRY);
    return;
  }

  ledger_->database()->UpdateCredsBatchStatus(
      trigger.id,
      trigger.type,
      ledger::CredsBatchStatus::FINISHED,
      callback);
}

}  // namespace braveledger_credentials
