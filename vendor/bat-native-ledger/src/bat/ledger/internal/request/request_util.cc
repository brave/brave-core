/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/strings/stringprintf.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/common/security_helper.h"

namespace {

std::string BuildBalanceUrl() {
  std::string url;
  switch (ledger::_environment) {
    case ledger::Environment::STAGING:
      url = BALANCE_STAGING_SERVER;
      break;
    case ledger::Environment::PRODUCTION:
      url = BALANCE_PRODUCTION_SERVER;
      break;
    case ledger::Environment::DEVELOPMENT:
      url = BALANCE_DEVELOPMENT_SERVER;
      break;
  }

  return url;
}

std::string BuildPublisherUrl() {
  std::string url;
  switch (ledger::_environment) {
    case ledger::Environment::STAGING:
      url = PUBLISHER_STAGING_SERVER;
      break;
    case ledger::Environment::PRODUCTION:
      url = PUBLISHER_PRODUCTION_SERVER;
      break;
    case ledger::Environment::DEVELOPMENT:
      url = PUBLISHER_DEVELOPMENT_SERVER;
      break;
  }

  return url;
}

std::string BuildPublisherDistroUrl() {
  std::string url;
  switch (ledger::_environment) {
    case ledger::Environment::STAGING:
      url = PUBLISHER_DISTRO_STAGING_SERVER;
      break;
    case ledger::Environment::PRODUCTION:
      url = PUBLISHER_DISTRO_PRODUCTION_SERVER;
      break;
    case ledger::Environment::DEVELOPMENT:
      url = PUBLISHER_DISTRO_DEVELOPMENT_SERVER;
      break;
  }

  return url;
}

std::string BuildLedgerUrl() {
  std::string url;
  switch (ledger::_environment) {
    case ledger::Environment::STAGING:
      url = LEDGER_STAGING_SERVER;
      break;
    case ledger::Environment::PRODUCTION:
      url = LEDGER_PRODUCTION_SERVER;
      break;
    case ledger::Environment::DEVELOPMENT:
      url = LEDGER_DEVELOPMENT_SERVER;
      break;
  }

  return url;
}

std::string BuildPromotionUrl() {
  std::string url;
  switch (ledger::_environment) {
    case ledger::Environment::STAGING: {
      url = PROMOTION_STAGING_SERVER;
      break;
    }
    case ledger::Environment::PRODUCTION: {
      url = PROMOTION_PRODUCTION_SERVER;
      break;
    }
    case ledger::Environment::DEVELOPMENT: {
      url = PROMOTION_DEVELOPMENT_SERVER;
      break;
    }
  }

  return url;
}

}  // namespace

namespace braveledger_request_util {

std::string BuildUrl(
    const std::string& path,
    const std::string& prefix,
    const ServerTypes& server) {
  std::string url;
  switch (server) {
    case ServerTypes::BALANCE: {
      url = BuildBalanceUrl();
      break;
    }
    case ServerTypes::PUBLISHER: {
      url = BuildPublisherUrl();
      break;
    }
    case ServerTypes::PUBLISHER_DISTRO: {
      url = BuildPublisherDistroUrl();
      break;
    }
    case ServerTypes::LEDGER: {
      url = BuildLedgerUrl();
      break;
    }
    case ServerTypes::kPromotion: {
      url = BuildPromotionUrl();
      break;
    }
  }

  if (url.empty()) {
    NOTREACHED();
    return "";
  }

  return url + prefix + path;
}

std::string DigestValue(const std::string& body) {
  const auto body_sha256 = braveledger_helper::Security::GetSHA256(body);
  const auto body_sha256_base64 =
      braveledger_helper::Security::GetBase64(body_sha256);

  return base::StringPrintf("SHA-256=%s", body_sha256_base64.c_str());
}

std::string SignatureHeaderValue(
    const std::string& data,
    const std::string& body,
    const std::string key_id,
    const std::vector<uint8_t>& private_key,
    const bool idempotency_key) {
  DCHECK(!body.empty());
  DCHECK(!private_key.empty());

  auto digest_header_value = DigestValue(body);

  std::vector<std::map<std::string, std::string>> headers;
  headers.push_back({{"digest", digest_header_value}});
  if (idempotency_key) {
    headers.push_back({{"idempotency-key", data}});
  } else {
    headers.push_back({{"(request-target)", data}});
  }

  return braveledger_helper::Security::Sign(
      headers,
      key_id,
      private_key);
}

std::map<std::string, std::string> GetSignHeaders(
    const std::string& data,
    const std::string& body,
    const std::string& key_id,
    const std::vector<uint8_t>& private_key,
    const bool idempotency_key) {
  const std::string digest_header = DigestValue(body).c_str();
  const std::string signature_header = SignatureHeaderValue(
      data,
      body,
      key_id,
      private_key,
      idempotency_key).c_str();

  return {
    { "digest", digest_header },
    { "signature", signature_header}
  };
}

std::vector<std::string> BuildSignHeaders(
    const std::string& url,
    const std::string& body,
    const std::string& key_id,
    const std::vector<uint8_t>& private_key) {
  auto headers = GetSignHeaders(url, body, key_id, private_key);
  DCHECK_EQ(headers.size(), 2ul);

  const std::string digest_header = base::StringPrintf(
      "digest: %s",
      headers.at("digest").c_str());
  const std::string signature_header = base::StringPrintf(
      "signature: %s",
      headers.at("signature").c_str());

  const std::string accept_header = "accept: application/json";

  return {
    digest_header,
    signature_header,
    accept_header
  };
}

}  // namespace braveledger_request_util
