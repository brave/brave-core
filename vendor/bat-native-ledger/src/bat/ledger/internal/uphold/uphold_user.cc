/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

  User::User() :
    name(""),
    member_at(""),
    verified(false),
    status(UserStatus::EMPTY),
    bat_not_allowed(true) {}

  User::~User() {}

}  // namespace braveledger_uphold

namespace braveledger_uphold {

UpholdUser::UpholdUser(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

UpholdUser::~UpholdUser() {
}

void UpholdUser::Get(
    ledger::ExternalWalletPtr wallet,
    GetUserCallback callback) {
  const auto headers = RequestAuthorization(wallet->token);
  const std::string url = GetAPIUrl("/v0/me");

  auto user_callback = std::bind(&UpholdUser::OnGet,
                                 this,
                                 callback,
                                 _1,
                                 _2,
                                 _3);
  ledger_->LoadURL(
      url,
      headers,
      "",
      "",
      ledger::URL_METHOD::GET,
      user_callback);
}

void UpholdUser::OnGet(
    GetUserCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  User user;
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, user);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, user);
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, user);
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, user);
    return;
  }

  auto* name = dictionary->FindKey("name");
  if (name && name->is_string()) {
    user.name = name->GetString();
  }

  auto* member_at = dictionary->FindKey("memberAt");
  if (member_at && member_at->is_string()) {
    user.member_at = member_at->GetString();
    user.verified = !user.member_at.empty();
  }

  auto* currencies = dictionary->FindKey("currencies");
  if (currencies && currencies->is_list()) {
    const std::string currency = "BAT";
    auto bat_in_list = std::find(
        currencies->GetList().begin(),
        currencies->GetList().end(),
        base::Value(currency));
    user.bat_not_allowed = bat_in_list == currencies->GetList().end();
  }

  auto* status = dictionary->FindKey("status");
  if (status && status->is_string()) {
    user.status = GetStatus(status->GetString());
  }

  callback(ledger::Result::LEDGER_OK, user);
}

UserStatus UpholdUser::GetStatus(const std::string& status) {
  if (status == "pending") {
    return UserStatus::PENDING;
  }

  if (status == "restricted") {
    return UserStatus::RESTRICTED;
  }

  if (status == "blocked") {
    return UserStatus::BLOCKED;
  }

  if (status == "ok") {
    return UserStatus::OK;
  }

  return UserStatus::EMPTY;
}

}  // namespace braveledger_uphold
