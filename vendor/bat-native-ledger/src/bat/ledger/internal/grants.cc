/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/grants.h"

#include <algorithm>
#include <map>
#include <utility>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_grant {

Grants::Grants(bat_ledger::LedgerImpl* ledger) :
      ledger_(ledger) {
}

Grants::~Grants() {
}

void Grants::FetchGrants(const std::string& lang,
                         const std::string& forPaymentId,
                         const std::string& safetynet_token,
                         ledger::FetchGrantsCallback callback) {
  // make sure wallet/client state is sane here as this is the first
  // panel call.
  const std::string& wallet_payment_id = ledger_->GetPaymentId();
  const std::string& passphrase = ledger_->GetWalletPassphrase();
  if (wallet_payment_id.empty() || passphrase.empty()) {
    std::vector<ledger::GrantPtr> empty_grants;
    callback(ledger::Result::CORRUPTED_WALLET, std::move(empty_grants));
    braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
    ledger_->OnWalletProperties(ledger::Result::CORRUPTED_WALLET, properties);
    return;
  }

  std::string paymentId = forPaymentId;
  if (paymentId.empty()) {
    paymentId = ledger_->GetPaymentId();
  }
  std::string arguments;
  if (!paymentId.empty() || !lang.empty()) {
    arguments = "?";
    if (!paymentId.empty()) {
      arguments += "paymentId=" + paymentId;
    }
    if (!lang.empty()) {
      if (arguments.length() > 1) {
        arguments += "&";
      }
      arguments += "lang=" + lang;
    }
  }

  std::vector<std::string> headers;
  if (!safetynet_token.empty()) {
    headers.push_back("safetynet-token:" + safetynet_token);
  }
  std::string safetynet_prefix = PREFIX_V5;
#if defined (OS_ANDROID) && defined(ARCH_CPU_X86_FAMILY) && defined(OFFICIAL_BUILD)
    safetynet_prefix = PREFIX_V3;
#endif
  auto internal_callback = std::bind(
      &Grants::GetGrantsCallback,
      this,
      safetynet_token,
      _1, _2, _3,
      std::move(callback));
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(
          (std::string)GET_SET_PROMOTION + arguments, safetynet_token.empty() ? PREFIX_V4 : safetynet_prefix),
      headers,
      "",
      "",
      ledger::URL_METHOD::GET,
      internal_callback);
}

void Grants::GetGrantsCallback(
    std::string safetynet_token,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::FetchGrantsCallback callback) {
  braveledger_bat_helper::GRANT properties;
  braveledger_bat_helper::Grants grants;
  braveledger_bat_helper::GRANTS_PROPERTIES_ST grants_properties;

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  unsigned int statusCode;
  std::string error;
  bool hasResponseError = braveledger_bat_helper::getJSONResponse(response,
                                                                  &statusCode,
                                                                  &error);
  if (hasResponseError && statusCode == net::HTTP_NOT_FOUND) {
    ledger_->SetLastGrantLoadTimestamp(time(0));
    ledger_->OnGrants(ledger::Result::GRANT_NOT_FOUND, grants, callback);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    ledger_->OnGrants(ledger::Result::LEDGER_ERROR, grants, callback);
    return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(&grants_properties, response);

  if (!ok && !safetynet_token.empty()) {
    ok = braveledger_bat_helper::loadFromJson(&properties, response);
    if (ok) {
      braveledger_bat_helper::GRANT_RESPONSE grantResponse;
      grantResponse.promotionId = properties.promotionId;
      grantResponse.type = properties.type;
      grants_properties.grants_.push_back(grantResponse);
    }
  }

  if (!ok) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Failed to load grant properties state";
    ledger_->OnGrants(ledger::Result::LEDGER_ERROR, grants, callback);
    return;
  }

  for (auto grant : grants_properties.grants_) {
    braveledger_bat_helper::GRANT grant_;
    grant_.promotionId = grant.promotionId;
    grant_.type = grant.type;
    grants.push_back(grant_);
  }

  ledger_->OnGrants(ledger::Result::LEDGER_OK, grants, callback);
  ledger_->SetLastGrantLoadTimestamp(time(0));
  ledger_->SetGrants(grants);
}

void Grants::SetGrant(const std::string& captchaResponse,
                      const std::string& promotionId,
                      const std::string& safetynet_token) {
  if (promotionId.empty() && safetynet_token.empty()) {
    braveledger_bat_helper::GRANT properties;
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promotionId, captchaResponse};
  std::string payload = braveledger_bat_helper::stringify(keys, values, safetynet_token.empty() ? 2 : 1);

  std::vector<std::string> headers;
  if (!safetynet_token.empty()) {
    headers.push_back("safetynet-token:" + safetynet_token);
  }

  auto callback = std::bind(&Grants::SetGrantCallback, this, _1, _2, _3, !safetynet_token.empty());
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_SET_PROMOTION + "/" + ledger_->GetPaymentId(),
        safetynet_token.empty() ? PREFIX_V2 : PREFIX_V3),
      headers, payload, "application/json; charset=utf-8",
      ledger::URL_METHOD::PUT, callback);
}

void Grants::SetGrantCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    bool is_safetynet_check) {
  std::string error;
  unsigned int statusCode;
  braveledger_bat_helper::GRANT grant;
  braveledger_bat_helper::getJSONResponse(response, &statusCode, &error);

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (!error.empty()) {
    if (statusCode == net::HTTP_FORBIDDEN) {
      ledger_->OnGrantFinish(is_safetynet_check ? ledger::Result::SAFETYNET_ATTESTATION_FAILED : ledger::Result::CAPTCHA_FAILED, grant);
    } else if (statusCode == net::HTTP_NOT_FOUND ||
               statusCode == net::HTTP_GONE) {
      ledger_->OnGrantFinish(ledger::Result::GRANT_NOT_FOUND, grant);
    } else if (statusCode == net::HTTP_CONFLICT) {
      ledger_->OnGrantFinish(ledger::Result::GRANT_ALREADY_CLAIMED, grant);
    } else {
      ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    }
    return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(&grant, response);
  if (!ok) {
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    return;
  }

  braveledger_bat_helper::Grants updated_grants;
  braveledger_bat_helper::Grants state_grants = ledger_->GetGrants();

  for (auto state_grant : state_grants) {
    if (grant.type == state_grant.type) {
      grant.promotionId = state_grant.promotionId;
      ledger_->OnGrantFinish(ledger::Result::LEDGER_OK, grant);
      updated_grants.push_back(grant);
    } else {
      updated_grants.push_back(state_grant);
    }
  }

  ledger_->SetGrants(updated_grants);
}

void Grants::GetGrantCaptcha(const std::vector<std::string>& headers,
    ledger::GetGrantCaptchaCallback callback) {
  auto on_load = std::bind(&Grants::GetGrantCaptchaCallback,
                            this,
                            _1,
                            _2,
                            _3,
                            std::move(callback));
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_PROMOTION_CAPTCHA + ledger_->GetPaymentId(),
        PREFIX_V4),
      headers, "", "", ledger::URL_METHOD::GET, std::move(on_load));
}

void Grants::GetGrantCaptchaCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::GetGrantCaptchaCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  auto it = headers.find("captcha-hint");
  if (response_status_code != net::HTTP_OK || it == headers.end()) {
    // TODO(nejczdovc): Add error handler
    return;
  }

  callback(response, it->second);
}

}  // namespace braveledger_grant
