/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_account/post_account_gemini.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal::endpoint::gemini {

PostAccount::PostAccount(LedgerImpl& ledger) : ledger_(ledger) {}

PostAccount::~PostAccount() = default;

std::string PostAccount::GetUrl() {
  return GetApiServerUrl("/v1/account");
}

mojom::Result PostAccount::ParseBody(const std::string& body,
                                     std::string* linking_info,
                                     std::string* user_name) {
  DCHECK(linking_info);
  DCHECK(user_name);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const base::Value::Dict* account = dict.FindDict("account");
  if (!account) {
    BLOG(0, "Missing account info");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* linking_information = account->FindString("verificationToken");
  if (!linking_info) {
    BLOG(0, "Missing linking info");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* users = dict.FindList("users");
  if (!users) {
    BLOG(0, "Missing users");
    return mojom::Result::LEDGER_ERROR;
  }

  if (users->size() == 0) {
    BLOG(0, "No users associated with this token");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* name = (*users)[0].GetDict().FindString("name");
  if (!name) {
    BLOG(0, "Missing user name");
    return mojom::Result::LEDGER_ERROR;
  }

  *linking_info = *linking_information;
  *user_name = *name;

  return mojom::Result::LEDGER_OK;
}

void PostAccount::Request(const std::string& token,
                          PostAccountCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  request->method = mojom::UrlMethod::POST;

  ledger_->LoadURL(std::move(request),
                   base::BindOnce(&PostAccount::OnRequest,
                                  base::Unretained(this), std::move(callback)));
}

void PostAccount::OnRequest(PostAccountCallback callback,
                            mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::LEDGER_OK) {
    return std::move(callback).Run(result, "", "");
  }

  std::string linking_info;
  std::string user_name;
  result = ParseBody(response->body, &linking_info, &user_name);
  std::move(callback).Run(result, std::move(linking_info),
                          std::move(user_name));
}

}  // namespace brave_rewards::internal::endpoint::gemini
