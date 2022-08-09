/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

#include <utility>

#include "base/json/json_writer.h"

namespace brave_wallet {

namespace internal {

base::Value::Dict ComposeRpcDict(base::StringPiece method) {
  base::Value::Dict dict;
  dict.Set("jsonrpc", "2.0");
  dict.Set("method", method);
  // I don't think we need to use this param, but it is required,
  // so always set it to 1 for now..
  dict.Set("id", 1);
  return dict;
}

}  // namespace internal

std::string GetJSON(base::ValueView dict) {
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

void AddKeyIfNotEmpty(base::Value::Dict* dict,
                      base::StringPiece name,
                      base::StringPiece val) {
  if (!val.empty()) {
    dict->Set(name, val);
  }
}

}  // namespace brave_wallet
