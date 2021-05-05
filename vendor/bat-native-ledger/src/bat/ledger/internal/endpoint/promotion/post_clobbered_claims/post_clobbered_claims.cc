/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_clobbered_claims/post_clobbered_claims.h"

#include <utility>

#include "base/json/json_writer.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PostClobberedClaims::PostClobberedClaims(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostClobberedClaims::~PostClobberedClaims() = default;

std::string PostClobberedClaims::GetUrl() {
  return GetServerUrl("/v2//promotions/reportclobberedclaims");
}

std::string PostClobberedClaims::GeneratePayload(base::Value corrupted_claims) {
  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("claimIds", std::move(corrupted_claims));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

type::Result PostClobberedClaims::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PostClobberedClaims::Request(
    base::Value corrupted_claims,
    PostClobberedClaimsCallback callback) {
  auto url_callback = std::bind(&PostClobberedClaims::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(std::move(corrupted_claims));
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostClobberedClaims::OnRequest(
    const type::UrlResponse& response,
    PostClobberedClaimsCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
