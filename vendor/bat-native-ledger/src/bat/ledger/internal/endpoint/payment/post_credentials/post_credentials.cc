/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/post_credentials/post_credentials.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/payment/payment_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace payment {

PostCredentials::PostCredentials(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostCredentials::~PostCredentials() = default;

std::string PostCredentials::GetUrl(const std::string& order_id) {
  const std::string path = base::StringPrintf(
      "/v1/orders/%s/credentials",
      order_id.c_str());

  return GetServerUrl(path);
}

std::string PostCredentials::GeneratePayload(
    const std::string& item_id,
    const std::string& type,
    base::Value::List&& blinded_creds) {
  base::Value::Dict body;
  body.Set("itemId", item_id);
  body.Set("type", type);
  body.Set("blindedCreds", std::move(blinded_creds));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostCredentials::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Credentials already exist for this order");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

void PostCredentials::Request(const std::string& order_id,
                              const std::string& item_id,
                              const std::string& type,
                              base::Value::List&& blinded_creds,
                              PostCredentialsCallback callback) {
  auto url_callback = base::BindOnce(
      &PostCredentials::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(order_id);
  request->content = GeneratePayload(item_id, type, std::move(blinded_creds));
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void PostCredentials::OnRequest(PostCredentialsCallback callback,
                                const mojom::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);
  std::move(callback).Run(CheckStatusCode(response.status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
