/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_REQUESTS_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_REQUESTS_HELPER_H_

#include <string>
#include "base/values.h"

// Helper functions for building out JSON RPC requests across all blockchains.
namespace brave_wallet {

base::Value GetJsonRpcDictionary(const std::string& method,
                                 base::Value* params);

std::string GetJSON(const base::Value& dictionary);

std::string GetJsonRpcNoParams(const std::string& method);

std::string GetJsonRpc1Param(const std::string& method, const std::string& val);

std::string GetJsonRpc2Params(const std::string& method,
                              const std::string& val1,
                              const std::string& val2);

std::string GetJsonRpc3Params(const std::string& method,
                              const std::string& val1,
                              const std::string& val2,
                              const std::string& val3);

void AddKeyIfNotEmpty(base::Value* dict,
                      const std::string& name,
                      const std::string& val);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_REQUESTS_HELPER_H_
