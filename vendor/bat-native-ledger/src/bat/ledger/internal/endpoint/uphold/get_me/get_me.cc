/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/get_me/get_me.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace uphold {

GetMe::GetMe(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetMe::~GetMe() = default;

std::string GetMe::GetUrl() {
  return GetServerUrl("/v0/me");
}

type::Result GetMe::CheckStatusCode(const int status_code) {
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

type::Result GetMe::ParseBody(
    const std::string& body,
    ::ledger::uphold::User* user) {
  DCHECK(user);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* name = dict.FindString("firstName");
  if (name) {
    user->name = *name;
  }

  if (const auto* id = dict.FindString("id")) {
    user->member_id = *id;
  }

  const auto* currencies = dict.FindList("currencies");
  if (currencies) {
    const std::string currency = "BAT";
    auto bat_in_list = std::find(currencies->begin(), currencies->end(),
                                 base::Value(currency));
    user->bat_not_allowed = bat_in_list == currencies->end();
  }

  return type::Result::LEDGER_OK;
}

void GetMe::Request(const std::string& token, GetMeCallback callback) {
  auto url_callback = base::BindOnce(&GetMe::OnRequest, base::Unretained(this),
                                     std::move(callback));

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void GetMe::OnRequest(GetMeCallback callback,
                      const type::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response, true);

  ::ledger::uphold::User user;
  type::Result result = CheckStatusCode(response.status_code);
  if (result != type::Result::LEDGER_OK) {
    std::move(callback).Run(result, user);
    return;
  }

  result = ParseBody(response.body, &user);
  std::move(callback).Run(result, user);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
