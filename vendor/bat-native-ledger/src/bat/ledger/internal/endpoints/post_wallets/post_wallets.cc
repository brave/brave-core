/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/post_wallets/post_wallets.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::endpoints {
using Error = PostWallets::Error;
using Result = PostWallets::Result;

namespace {

Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto* payment_id = value->GetDict().FindString("paymentId");
  if (!payment_id || payment_id->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return *payment_id;
}

}  // namespace

// static
Result PostWallets::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_CREATED:  // HTTP 201
      return ParseBody(response.body);
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      BLOG(0, "Invalid request!");
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Invalid public key!");
      return base::unexpected(Error::kInvalidPublicKey);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      BLOG(0, "Wallet generation disabled!");
      return base::unexpected(Error::kWalletGenerationDisabled);
    case net::HTTP_CONFLICT:  // HTTP 409
      BLOG(0, "Wallet already exists!");
      return base::unexpected(Error::kWalletAlreadyExists);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      BLOG(0, "Unexpected error!");
      return base::unexpected(Error::kUnexpectedError);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

PostWallets::PostWallets(LedgerImpl* ledger,
                         absl::optional<std::string>&& geo_country)
    : RequestBuilder(ledger), geo_country_(std::move(geo_country)) {}

PostWallets::~PostWallets() = default;

const char* PostWallets::Path() const {
  return geo_country_ ? "/v4/wallets" : "/v3/wallet/brave";
}

absl::optional<std::string> PostWallets::Url() const {
  return endpoint::promotion::GetServerUrl(Path());
}

absl::optional<std::vector<std::string>> PostWallets::Headers(
    const std::string& content) const {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->recovery_seed.empty());

  return util::BuildSignHeaders(
      std::string("post ") + Path(), content,
      util::Security::GetPublicKeyHexFromSeed(wallet->recovery_seed),
      wallet->recovery_seed);
}

absl::optional<std::string> PostWallets::Content() const {
  if (!geo_country_) {
    BLOG(1, "geo_country_ is null - creating old wallet.");
    return "";
  }

  if (geo_country_->empty()) {
    BLOG(0, "geo_country_ is empty!");
    return absl::nullopt;
  }

  base::Value::Dict content;
  content.Set("geoCountry", *geo_country_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return absl::nullopt;
  }

  return json;
}

std::string PostWallets::ContentType() const {
  return kApplicationJson;
}

}  // namespace ledger::endpoints
