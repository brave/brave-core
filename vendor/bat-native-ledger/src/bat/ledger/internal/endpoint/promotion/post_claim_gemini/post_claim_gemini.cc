/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/promotion/post_claim_gemini/post_claim_gemini.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/mojom_structs.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

std::string GetPath(const std::string& payment_id) {
  return base::StringPrintf("/v3/wallet/gemini/%s/claim", payment_id.c_str());
}

}  // namespace

namespace ledger {
namespace endpoint {
namespace promotion {

PostClaimGemini::PostClaimGemini(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostClaimGemini::~PostClaimGemini() = default;

std::string PostClaimGemini::GetUrl() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string path = GetPath(wallet->payment_id);

  return GetServerUrl(path);
}

std::string PostClaimGemini::GeneratePayload(const std::string& linking_info,
                                             const std::string& recipient_id) {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("linking_info", linking_info);
  payload.SetStringKey("recipient_id", recipient_id);
  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

type::Result PostClaimGemini::ProcessResponse(
    const type::UrlResponse& response) const {
  const auto status_code = response.status_code;

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Forbidden");
    return ParseBody(response.body);
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Not found");
    return type::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Conflict");
    return type::Result::DEVICE_LIMIT_REACHED;
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

// disambiguating on the HTTP 403 (Forbidden)
type::Result PostClaimGemini::ParseBody(const std::string& body) const {
  base::DictionaryValue* root = nullptr;
  auto value = base::JSONReader::Read(body);
  if (!value || !value->GetAsDictionary(&root)) {
    BLOG(0, "Invalid body!");
    return type::Result::LEDGER_ERROR;
  }
  DCHECK(root);

  auto* message = root->FindStringKey("message");
  if (!message) {
    BLOG(0, "message is missing!");
    return type::Result::LEDGER_ERROR;
  }

  if (message->find("mismatched provider accounts") != std::string::npos) {
    return type::Result::MISMATCHED_PROVIDER_ACCOUNTS;
  } else if (message->find("request signature verification failure") !=
             std::string::npos) {
    return type::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE;
  } else {
    BLOG(0, "Unknown message!");
    return type::Result::LEDGER_ERROR;
  }
}

void PostClaimGemini::Request(const std::string& linking_info,
                              const std::string& recipient_id,
                              PostClaimGeminiCallback callback) {
  auto url_callback =
      std::bind(&PostClaimGemini::OnRequest, this, _1, callback);
  const std::string payload = GeneratePayload(linking_info, recipient_id);

  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const auto sign_url =
      base::StringPrintf("post %s", GetPath(wallet->payment_id).c_str());
  auto headers = util::BuildSignHeaders(sign_url, payload, wallet->payment_id,
                                        wallet->recovery_seed);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostClaimGemini::OnRequest(const type::UrlResponse& response,
                                PostClaimGeminiCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(ProcessResponse(response));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
