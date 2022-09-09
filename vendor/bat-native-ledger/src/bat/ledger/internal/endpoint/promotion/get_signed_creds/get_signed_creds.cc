/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_signed_creds/get_signed_creds.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace promotion {

GetSignedCreds::GetSignedCreds(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetSignedCreds::~GetSignedCreds() = default;

std::string GetSignedCreds::GetUrl(
    const std::string& promotion_id,
    const std::string& claim_id) {
  const std::string& path = base::StringPrintf(
      "/v1/promotions/%s/claims/%s",
      promotion_id.c_str(),
      claim_id.c_str());

  return GetServerUrl(path);
}

mojom::Result GetSignedCreds::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
    return mojom::Result::NOT_FOUND;
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

mojom::Result GetSignedCreds::ParseBody(const std::string& body,
                                        mojom::CredsBatch* batch) {
  DCHECK(batch);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* batch_proof = dict.FindString("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return mojom::Result::LEDGER_ERROR;
  }

  auto* signed_creds = dict.FindList("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return mojom::Result::LEDGER_ERROR;
  }

  auto* public_key = dict.FindString("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
    return mojom::Result::LEDGER_ERROR;
  }

  base::JSONWriter::Write(*signed_creds, &batch->signed_creds);
  batch->public_key = *public_key;
  batch->batch_proof = *batch_proof;

  return mojom::Result::LEDGER_OK;
}

void GetSignedCreds::Request(
    const std::string& promotion_id,
    const std::string& claim_id,
    GetSignedCredsCallback callback) {
  auto url_callback = base::BindOnce(
      &GetSignedCreds::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(promotion_id, claim_id);
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void GetSignedCreds::OnRequest(GetSignedCredsCallback callback,
                               const mojom::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);

  mojom::Result result = CheckStatusCode(response.status_code);

  if (result != mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  mojom::CredsBatch batch;
  result = ParseBody(response.body, &batch);
  std::move(callback).Run(result, mojom::CredsBatch::New(batch));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
