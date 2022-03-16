/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace {
const char kRequestJsonRPC[] = "2.0";
}  // namespace

namespace brave_wallet {

std::unique_ptr<base::Value> GetProviderErrorDictionary(
    mojom::ProviderError code,
    const std::string& message) {
  std::string formed_response;
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  result->SetIntKey("code", static_cast<int>(code));
  result->SetStringKey("message", message);
  return std::move(result);
}

std::unique_ptr<base::Value> GetProviderRequestReturnFromEthJsonResponse(
    int http_code,
    const std::string& service_response,
    bool* reject) {
  DCHECK(reject);
  *reject = true;
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          service_response, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;

  if (http_code != 200) {
    mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
    std::string message =
        "HTTP Status code: " + base::NumberToString(http_code);
    return GetProviderErrorDictionary(code, message);
  }

  if (!response) {
    mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
    std::string message =
        "Invalid response, could not parse JSON: " + service_response;

    return GetProviderErrorDictionary(code, message);
  }

  const base::Value* error = response->FindKey("error");
  if (error) {
    return base::Value::ToUniquePtrValue(error->Clone());
  }

  // We have a result
  const base::Value* result = response->FindKey("result");
  DCHECK(result);

  *reject = false;
  return base::Value::ToUniquePtrValue(result->Clone());
}

std::unique_ptr<base::Value> ToProviderResponse(base::Value id,
                                                base::Value* result,
                                                base::Value* error) {
  base::Value response(base::Value::Type::DICTIONARY);

  response.SetKey("id", std::move(id));
  response.SetStringKey("jsonrpc", kRequestJsonRPC);

  if (result) {
    response.SetKey("result", result->Clone());
  }

  if (error) {
    response.SetKey("error", error->Clone());
  }

  return base::Value::ToUniquePtrValue(std::move(response));
}

}  // namespace brave_wallet
