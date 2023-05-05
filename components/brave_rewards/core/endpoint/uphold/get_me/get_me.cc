/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_me/get_me.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace uphold {

GetMe::GetMe(LedgerImpl& ledger) : ledger_(ledger) {}

GetMe::~GetMe() = default;

std::string GetMe::GetUrl() {
  return GetServerUrl("/v0/me");
}

mojom::Result GetMe::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result GetMe::ParseBody(const std::string& body,
                               internal::uphold::User* user) {
  DCHECK(user);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
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
    user->bat_not_allowed = !base::Contains(*currencies, base::Value(currency));
  }

  return mojom::Result::LEDGER_OK;
}

void GetMe::Request(const std::string& token, GetMeCallback callback) {
  auto url_callback = base::BindOnce(&GetMe::OnRequest, base::Unretained(this),
                                     std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void GetMe::OnRequest(GetMeCallback callback, mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response, true);

  internal::uphold::User user;
  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, user);
    return;
  }

  result = ParseBody(response->body, &user);
  std::move(callback).Run(result, user);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal
