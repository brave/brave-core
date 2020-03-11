/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/credentials/credentials_sku.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

const char public_key_dev[] =
    "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=";
const char public_key_staging[] =
    "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=";
const char public_key_production[] =
    "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=";

bool VerifyPublicKey(const std::string& public_key) {
  std::string order_key = "";
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    order_key = public_key_production;
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    order_key = public_key_staging;
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    order_key = public_key_dev;
  }

  return order_key == public_key;
}

std::string ConvertItemTypeToString(const std::string& type) {
  switch (static_cast<ledger::SKUOrderItemType>(std::stoi(type))) {
    case ledger::SKUOrderItemType::SINGLE_USE: {
      return "single-use";
    }
    case ledger::SKUOrderItemType::NONE: {
      return "";
    }
  }
}

}  // namespace

namespace braveledger_credentials {

CredentialsSKU::CredentialsSKU(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<CredentialsCommon>(ledger))  {
  DCHECK(ledger_ && common_);
}

CredentialsSKU::~CredentialsSKU() = default;

void CredentialsSKU::Start(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  DCHECK_EQ(trigger.data.size(), 2ul);
  if (trigger.data.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Trigger data is missing";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::OnStart,
          this,
          _1,
          trigger,
          callback);

  ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
}

void CredentialsSKU::OnStart(
    ledger::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger::CredsBatchStatus status = ledger::CredsBatchStatus::NONE;
  if (creds) {
    status = creds->status;
  }

  switch (status) {
    case ledger::CredsBatchStatus::NONE:
    case ledger::CredsBatchStatus::BLINDED: {
      Blind(trigger, callback);
      break;
    }
    case ledger::CredsBatchStatus::CLAIMED: {
      FetchSignedCreds(trigger, callback);
      break;
    }
    case ledger::CredsBatchStatus::SIGNED: {
      auto get_callback = std::bind(&CredentialsSKU::Unblind,
          this,
          _1,
          trigger,
          callback);
      ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
      break;
    }
    case ledger::CredsBatchStatus::FINISHED: {
      callback(ledger::Result::LEDGER_OK);
      break;
    }
  }
}

void CredentialsSKU::Blind(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto blinded_callback = std::bind(&CredentialsSKU::Claim,
      this,
      _1,
      _2,
      trigger,
      callback);
  common_->GetBlindedCreds(trigger, blinded_callback);
}

void CredentialsSKU::Claim(
    const ledger::Result result,
    const std::string& blinded_creds_json,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(blinded_creds_json);

  if (!blinded_creds || blinded_creds->empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Blinded creds are corrupted";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("itemId", trigger.data[0]);
  body.SetStringKey("type", ConvertItemTypeToString(trigger.data[1]));
  body.SetKey("blindedCreds", base::Value(std::move(*blinded_creds)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();

  const std::string sign_url = base::StringPrintf(
      "post /v1/orders/%s/credentials",
      trigger.id.c_str());

  const auto headers = braveledger_request_util::BuildSignHeaders(
      sign_url,
      json,
      ledger_->GetPaymentId(),
      wallet_info.key_info_seed);

  const std::string url =
      braveledger_request_util::GetOrderCredentialsURL(trigger.id);
  auto url_callback = std::bind(&CredentialsSKU::OnClaim,
      this,
      _1,
      _2,
      _3,
      trigger,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void CredentialsSKU::OnClaim(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&CredentialsSKU::ClaimStatusSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->UpdateCredsBatchStatus(
      trigger.id,
      trigger.type,
      ledger::CredsBatchStatus::CLAIMED,
      save_callback);
}

void CredentialsSKU::ClaimStatusSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Claim status not saved";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  FetchSignedCreds(trigger, callback);
}

void CredentialsSKU::FetchSignedCreds(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  const std::string url = braveledger_request_util::GetOrderCredentialsURL(
          trigger.id,
          trigger.data[0]);
  auto url_callback = std::bind(&CredentialsSKU::OnFetchSignedCreds,
      this,
      _1,
      _2,
      _3,
      trigger,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void CredentialsSKU::OnFetchSignedCreds(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_ACCEPTED) {
    callback(ledger::Result::RETRY);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::SignedCredsSaved,
      this,
      _1,
      trigger,
      callback);
  common_->GetSignedCredsFromResponse(trigger, response, get_callback);
}

void CredentialsSKU::SignedCredsSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Signed creds were not saved";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::Unblind,
      this,
      _1,
      trigger,
      callback);
  ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
}

void CredentialsSKU::Unblind(
    ledger::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Corrupted data";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (VerifyPublicKey(creds->public_key)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Public key is not valid";
    callback(ledger::Result::LEDGER_ERROR);
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
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << error;
    callback(ledger::Result::LEDGER_ERROR);
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
      braveledger_ledger::_vote_price,
      *creds,
      unblinded_encoded_creds,
      trigger,
      save_callback);
}

void CredentialsSKU::Completed(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Unblinded token save failed";
    callback(result);
    return;
  }

  ledger_->UnblindedTokensReady();
  callback(result);
}

}  // namespace braveledger_credentials
