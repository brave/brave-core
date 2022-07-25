/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace {
const char kRequestJsonRPC[] = "2.0";

base::Value GetProviderErrorDictionaryIternal(int code,
                                              const std::string& message) {
  std::string formed_response;
  base::Value::Dict result;
  result.Set("code", code);
  result.Set("message", message);
  return base::Value(std::move(result));
}

}  // namespace

base::Value GetProviderErrorDictionary(mojom::ProviderError code,
                                       const std::string& message) {
  return GetProviderErrorDictionaryIternal(static_cast<int>(code), message);
}

base::Value GetSolanaProviderErrorDictionary(mojom::SolanaProviderError code,
                                             const std::string& message) {
  return GetProviderErrorDictionaryIternal(static_cast<int>(code), message);
}

base::Value GetProviderRequestReturnFromEthJsonResponse(
    int http_code,
    const std::string& service_response,
    bool* reject) {
  DCHECK(reject);
  *reject = true;

  if (http_code != 200) {
    mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
    std::string message =
        "HTTP Status code: " + base::NumberToString(http_code);
    return GetProviderErrorDictionary(code, message);
  }

  absl::optional<base::Value> response = base::JSONReader::Read(
      service_response, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                            base::JSONParserOptions::JSON_PARSE_RFC);
  if (!response || !response->is_dict()) {
    mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
    std::string message =
        "Invalid response, could not parse JSON: " + service_response;

    return GetProviderErrorDictionary(code, message);
  }

  auto& dict = response->GetDict();
  base::Value* error = dict.Find("error");
  if (error) {
    return std::move(*error);
  }

  // We have a result
  base::Value* result = dict.Find("result");
  DCHECK(result);

  *reject = false;
  return std::move(*result);
}

base::Value ToProviderResponse(base::Value id,
                               base::Value* result,
                               base::Value* error) {
  base::Value::Dict response;

  response.Set("id", std::move(id));
  response.Set("jsonrpc", kRequestJsonRPC);

  if (result) {
    response.Set("result", result->Clone());
  }

  if (error) {
    response.Set("error", error->Clone());
  }

  return base::Value(std::move(response));
}

}  // namespace brave_wallet
