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

void Grants::GetGrants(const std::string& lang,
                       const std::string& forPaymentId) {
  // make sure wallet/client state is sane here as this is the first
  // panel call.
  const std::string& wallet_payment_id = ledger_->GetPaymentId();
  const std::string& passphrase = ledger_->GetWalletPassphrase();
  if (wallet_payment_id.empty() || passphrase.empty()) {
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

  auto callback = std::bind(&Grants::GetGrantsCallback, this, _1, _2, _3);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_SET_PROMOTION + arguments, PREFIX_V4),
      std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, callback);
}

void Grants::GetGrantsCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
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
    ledger_->OnGrant(ledger::Result::GRANT_NOT_FOUND, properties);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    ledger_->OnGrant(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(&grants_properties, response);

  if (!ok) {
     BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
       "Failed to load grant properties state";
    ledger_->OnGrant(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  for (auto grant : grants_properties.grants_) {
    braveledger_bat_helper::GRANT grant_;
    grant_.promotionId = grant.promotionId;
    grant_.type = grant.type;

    grants.push_back(grant_);
    ledger_->OnGrant(ledger::Result::LEDGER_OK, grant_);
  }

  ledger_->SetLastGrantLoadTimestamp(time(0));
  ledger_->SetGrants(grants);
}

void Grants::SetGrant(const std::string& captchaResponse,
                      const std::string& promotionId) {
  if (promotionId.empty()) {
    braveledger_bat_helper::GRANT properties;
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promotionId, captchaResponse};
  std::string payload = braveledger_bat_helper::stringify(keys, values, 2);

  auto callback = std::bind(&Grants::SetGrantCallback, this, _1, _2, _3);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_SET_PROMOTION + "/" + ledger_->GetPaymentId(),
        PREFIX_V2),
      std::vector<std::string>(), payload, "application/json; charset=utf-8",
      ledger::URL_METHOD::PUT, callback);
}

void Grants::SetGrantCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  std::string error;
  unsigned int statusCode;
  braveledger_bat_helper::GRANT grant;
  braveledger_bat_helper::getJSONResponse(response, &statusCode, &error);

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (!error.empty()) {
    if (statusCode == net::HTTP_FORBIDDEN) {
      ledger_->OnGrantFinish(ledger::Result::CAPTCHA_FAILED, grant);
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

void Grants::GetGrantCaptcha(const std::vector<std::string>& headers) {
  auto callback = std::bind(&Grants::GetGrantCaptchaCallback,
                            this,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_PROMOTION_CAPTCHA + ledger_->GetPaymentId(),
        PREFIX_V4),
      headers, "", "", ledger::URL_METHOD::GET, callback);
}

void Grants::GetGrantCaptchaCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  auto it = headers.find("captcha-hint");
  if (response_status_code != net::HTTP_OK || it == headers.end()) {
    // TODO(nejczdovc): Add error handler
    return;
  }

  ledger_->OnGrantCaptcha(response, it->second);
}

}  // namespace braveledger_grant
