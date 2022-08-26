/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/patch_card/patch_card.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace uphold {

PatchCard::PatchCard(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PatchCard::~PatchCard() = default;

std::string PatchCard::GetUrl(const std::string& address) {
  return GetServerUrl("/v0/me/cards/" + address);
}

std::string PatchCard::GeneratePayload() {
  base::Value::Dict settings;
  settings.Set("position", 1);
  settings.Set("starred", true);

  base::Value::Dict payload;
  payload.Set("settings", std::move(settings));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

type::Result PatchCard::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return type::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PatchCard::Request(const std::string& token,
                        const std::string& address,
                        PatchCardCallback callback) {
  auto url_callback = base::BindOnce(
      &PatchCard::OnRequest, base::Unretained(this), std::move(callback));

  auto request = type::UrlRequest::New();
  request->url = GetUrl(address);
  request->content = GeneratePayload();
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::PATCH;
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void PatchCard::OnRequest(PatchCardCallback callback,
                          const type::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);
  std::move(callback).Run(CheckStatusCode(response.status_code));
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
