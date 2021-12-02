/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_requests.h"

#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace {

base::Value GetJsonRpcDictionary(const std::string& method,
                                 base::Value* params) {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetKey("jsonrpc", base::Value("2.0"));
  dictionary.SetKey("method", base::Value(method));
  dictionary.SetKey("params", std::move(*params));
  // I don't think we need to use this param, but it is required,
  // so always set it to 1 for now..
  dictionary.SetKey("id", base::Value(1));
  return dictionary;
}

std::string GetJSON(const base::Value& dictionary) {
  std::string json;
  base::JSONWriter::Write(dictionary, &json);
  return json;
}


std::string GetJsonRpc2Params(const std::string& method,
                              const std::string& val1,
                              const std::string& val2) {
  base::Value params(base::Value::Type::LIST);
  params.Append(base::Value(val1));
  params.Append(base::Value(val2));
  base::Value dictionary = GetJsonRpcDictionary(method, &params);
  return GetJSON(dictionary);
}

}  // namespace

namespace brave_wallet {


std::string fil_getBalance(const std::string& address,
                           const std::string& quantity_tag) {
  return GetJsonRpc2Params("Filecoin.WalletBalance", address, quantity_tag);
}

}  // namespace brave_wallet
