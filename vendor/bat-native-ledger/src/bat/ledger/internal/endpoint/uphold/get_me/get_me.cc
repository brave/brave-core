/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/get_me/get_me.h"

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

braveledger_uphold::UserStatus GetUserStatus(const std::string& status) {
  if (status == "pending") {
    return braveledger_uphold::UserStatus::PENDING;
  }

  if (status == "restricted") {
    return braveledger_uphold::UserStatus::RESTRICTED;
  }

  if (status == "blocked") {
    return braveledger_uphold::UserStatus::BLOCKED;
  }

  if (status == "ok") {
    return braveledger_uphold::UserStatus::OK;
  }

  return braveledger_uphold::UserStatus::EMPTY;
}

}  // namespace

namespace ledger {
namespace endpoint {
namespace uphold {

GetMe::GetMe(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetMe::~GetMe() = default;

std::string GetMe::GetUrl() {
  return GetServerUrl("/v0/me");
}

ledger::Result GetMe::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result GetMe::ParseBody(
    const std::string& body,
    braveledger_uphold::User* user) {
  DCHECK(user);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* name = dictionary->FindStringKey("firstName");
  if (name) {
    user->name = *name;
  }

  const auto* member_at = dictionary->FindStringKey("memberAt");
  if (member_at) {
    user->member_at = *member_at;
    user->verified = !user->member_at.empty();
  }

  const auto* currencies = dictionary->FindListKey("currencies");
  if (currencies) {
    const std::string currency = "BAT";
    auto bat_in_list = std::find(
        currencies->GetList().begin(),
        currencies->GetList().end(),
        base::Value(currency));
    user->bat_not_allowed = bat_in_list == currencies->GetList().end();
  }

  const auto* status = dictionary->FindStringKey("status");
  if (status) {
    user->status = GetUserStatus(*status);
  }

  return ledger::Result::LEDGER_OK;
}

void GetMe::Request(
    const std::string& token,
    GetMeCallback callback) {
  auto url_callback = std::bind(&GetMe::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      RequestAuthorization(token),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetMe::OnRequest(
    const ledger::UrlResponse& response,
    GetMeCallback callback) {
  ledger::LogUrlResponse(__func__, response, true);

  braveledger_uphold::User user;
  ledger::Result result = CheckStatusCode(response.status_code);
  if (result != ledger::Result::LEDGER_OK) {
    callback(result, user);
    return;
  }

  result = ParseBody(response.body, &user);
  callback(result, user);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
