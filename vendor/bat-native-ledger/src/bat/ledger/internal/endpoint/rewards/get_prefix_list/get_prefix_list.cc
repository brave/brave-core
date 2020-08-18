/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/rewards/get_prefix_list/get_prefix_list.h"

#include "bat/ledger/internal/endpoint/rewards/rewards_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// GET /publishers/prefix-list
//
// Success code:
// HTTP_OK (200)
//
// Response body:
// blob

namespace ledger {
namespace endpoint {
namespace rewards {

GetPrefixList::GetPrefixList(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetPrefixList::~GetPrefixList() = default;

std::string GetPrefixList::GetUrl() {
  return GetServerUrl("/publishers/prefix-list");
}

ledger::Result GetPrefixList::CheckStatusCode(const int status_code) {
  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

void GetPrefixList::Request(GetPrefixListCallback callback) {
  auto url_callback = std::bind(&GetPrefixList::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(GetUrl(), {}, "", "", ledger::UrlMethod::GET, url_callback);
}

void GetPrefixList::OnRequest(
    const ledger::UrlResponse& response,
    GetPrefixListCallback callback) {
  ledger::LogUrlResponse(__func__, response, true);

  if (CheckStatusCode(response.status_code) != ledger::Result::LEDGER_OK ||
      response.body.empty()) {
    BLOG(0, "Invalid server response for publisher prefix list");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, response.body);
}

}  // namespace rewards
}  // namespace endpoint
}  // namespace ledger
