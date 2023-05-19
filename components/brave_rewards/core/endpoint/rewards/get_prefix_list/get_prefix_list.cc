/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"

#include <utility>

#include "brave/components/brave_rewards/core/endpoint/rewards/rewards_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace brave_rewards::internal::endpoint::rewards {

std::string GetPrefixList::GetUrl() {
  return GetServerUrl("/publishers/prefix-list");
}

mojom::Result GetPrefixList::CheckStatusCode(const int status_code) {
  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

void GetPrefixList::Request(GetPrefixListCallback callback) {
  auto url_callback = std::bind(&GetPrefixList::OnRequest, this, _1, callback);

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  ledger().LoadURL(std::move(request), url_callback);
}

void GetPrefixList::OnRequest(mojom::UrlResponsePtr response,
                              GetPrefixListCallback callback) {
  DCHECK(response);
  LogUrlResponse(__func__, *response, true);

  if (CheckStatusCode(response->status_code) != mojom::Result::LEDGER_OK ||
      response->body.empty()) {
    BLOG(0, "Invalid server response for publisher prefix list");
    callback(mojom::Result::LEDGER_ERROR, "");
    return;
  }

  callback(mojom::Result::LEDGER_OK, response->body);
}

}  // namespace brave_rewards::internal::endpoint::rewards
