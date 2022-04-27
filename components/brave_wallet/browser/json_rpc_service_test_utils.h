/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_TEST_UTILS_H_

#include <string>
#include <vector>

namespace brave_wallet {

std::string MakeJsonRpcStringArrayResponse(
    const std::vector<std::string>& items);
std::string MakeJsonRpcStringResponse(const std::string& str);
std::string MakeJsonRpcErrorResponse(int error,
                                     const std::string& error_message);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_TEST_UTILS_H_
