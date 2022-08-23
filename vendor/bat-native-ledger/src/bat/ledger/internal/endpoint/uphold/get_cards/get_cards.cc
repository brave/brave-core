/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/get_cards/get_cards.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace uphold {

GetCards::GetCards(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetCards::~GetCards() = default;

std::string GetCards::GetUrl() {
  return GetServerUrl("/v0/me/cards?q=currency:BAT");
}

type::Result GetCards::CheckStatusCode(const int status_code) {
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

type::Result GetCards::ParseBody(
    const std::string& body,
    std::string* id) {
  DCHECK(id);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  auto& list = value->GetList();
  for (const auto& it : list) {
    DCHECK(it.is_dict());
    const auto& dict = it.GetDict();
    const auto* label = dict.FindString("label");
    if (!label) {
      continue;
    }

    if (*label == ::ledger::uphold::kCardName) {
      const auto* id_str = dict.FindString("id");
      if (!id_str) {
        continue;
      }

      *id = *id_str;
      return type::Result::LEDGER_OK;
    }
  }

  return type::Result::LEDGER_ERROR;
}

void GetCards::Request(
    const std::string& token,
    GetCardsCallback callback) {
  auto url_callback = base::BindOnce(
      &GetCards::OnRequest, base::Unretained(this), std::move(callback));

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void GetCards::OnRequest(GetCardsCallback callback,
                         const type::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    std::move(callback).Run(result, "");
    return;
  }

  std::string id;
  result = ParseBody(response.body, &id);
  std::move(callback).Run(result, id);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
