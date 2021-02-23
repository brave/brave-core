/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion/bap_reporter_endpoint.h"

#include <string>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace promotion {

namespace {

const char kEndpointPath[] = "/v1/promotions/report-bap";

}  // namespace

BAPReporterEndpoint::BAPReporterEndpoint(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

BAPReporterEndpoint::~BAPReporterEndpoint() = default;

void BAPReporterEndpoint::Request(double amount, Callback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(false);
    return;
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetDoubleKey("amount", amount);
  std::string payload;
  base::JSONWriter::Write(body, &payload);

  const std::string sign_url = std::string("post ") + kEndpointPath;
  auto headers = util::BuildSignHeaders(sign_url, payload, wallet->payment_id,
                                        wallet->recovery_seed);

  auto request = type::UrlRequest::New();
  request->url = endpoint::promotion::GetServerUrl(kEndpointPath);
  request->headers = std::move(headers);
  request->content = std::move(payload);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(
      std::move(request),
      std::bind(&BAPReporterEndpoint::OnFetchCompleted, this, callback, _1));
}

void BAPReporterEndpoint::OnFetchCompleted(Callback callback,
                                           const type::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);

  bool success = false;
  switch (response.status_code) {
    case net::HTTP_BAD_REQUEST: {
      BLOG(0, "Invalid request");
      break;
    }
    case net::HTTP_INTERNAL_SERVER_ERROR: {
      BLOG(0, "Internal server error");
      break;
    }
    case net::HTTP_OK:
    case net::HTTP_CONFLICT: {
      success = true;
      break;
    }
    default: {
      BLOG(0, "Unexpected reponse code " << response.status_code);
      break;
    }
  }

  callback(success);
}

}  // namespace promotion
}  // namespace ledger
