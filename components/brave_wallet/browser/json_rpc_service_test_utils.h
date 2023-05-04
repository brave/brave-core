/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_TEST_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/eth_abi_utils.h"

namespace brave_wallet {
class EthAddress;

std::string MakeJsonRpcStringArrayResponse(
    const std::vector<std::string>& items);
std::string MakeJsonRpcStringResponse(const std::string& str);
std::string MakeJsonRpcTupleResponse(const eth_abi::TupleEncoder& tuple);
std::string MakeJsonRpcRawBytesResponse(const std::vector<uint8_t>& bytes);
std::string MakeJsonRpcErrorResponse(int error,
                                     const std::string& error_message);
std::string MakeJsonRpcErrorResponseWithData(int error,
                                             const std::string& error_message,
                                             const std::string& data);

std::string MakeJsonRpcValueResponse(const base::Value& value);
std::string MakeJsonRpcResultResponse(const base::Value& value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_TEST_UTILS_H_
