/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/post_account/post_account_gemini.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace gemini {

PostAccount::PostAccount(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostAccount::~PostAccount() = default;

std::string PostAccount::GetUrl() {
  return GetApiServerUrl("/v1/account");
}

type::Result PostAccount::ParseBody(const std::string& body,
                                    std::string* linking_info,
                                    std::string* user_name) {
  DCHECK(linking_info);
  DCHECK(user_name);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::Value* account = dictionary->FindDictKey("account");
  if (!account) {
    BLOG(0, "Missing account info");
    return type::Result::LEDGER_ERROR;
  }

  const auto* linking_information = account->FindStringKey("verificationToken");
  if (!linking_info) {
    BLOG(0, "Missing linking info");
    return type::Result::LEDGER_ERROR;
  }

  const auto* users = dictionary->FindListKey("users");
  if (!users) {
    BLOG(0, "Missing users");
    return type::Result::LEDGER_ERROR;
  }

  const base::Value::ConstListView user_list = users->GetListDeprecated();
  if (user_list.size() == 0) {
    BLOG(0, "No users associated with this token");
    return type::Result::LEDGER_ERROR;
  }

  const auto* name = user_list[0].FindStringKey("name");
  if (!name) {
    BLOG(0, "Missing user name");
    return type::Result::LEDGER_ERROR;
  }

  *linking_info = *linking_information;
  *user_name = *name;

  return type::Result::LEDGER_OK;
}

void PostAccount::Request(const std::string& token,
                          PostAccountCallback callback) {
  auto url_callback = std::bind(&PostAccount::OnRequest, this, _1, callback);
  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostAccount::OnRequest(const type::UrlResponse& response,
                            PostAccountCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, "", "");
    return;
  }

  std::string linking_info;
  std::string user_name;

  result = ParseBody(response.body, &linking_info, &user_name);
  callback(result, linking_info, user_name);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
