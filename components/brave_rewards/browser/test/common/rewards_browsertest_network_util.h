/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_NETWORK_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_NETWORK_UTIL_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/mojom_structs.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace rewards_browsertest_util {

bool URLMatches(
    const std::string& url,
    const std::string& path,
    const std::string& prefix,
    const braveledger_request_util::ServerTypes server);

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request);

std::string GetUpholdUser(const bool wallet_verified);

std::string GetUpholdCard(
    const std::string& balance,
    const std::string& address);

std::string GetOrderCreateResponse(ledger::SKUOrderPtr sku_order);

}  // namespace rewards_browsertest_util

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_NETWORK_UTIL_H_
