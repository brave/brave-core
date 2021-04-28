/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"

#include <utility>

#include "base/json/json_reader.h"

namespace brave_wallet {

std::unique_ptr<base::Value> FormProviderResponse(ProviderErrors code,
                                                  const std::string& message) {
  std::string formed_response;
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  result->SetIntKey("code", static_cast<int>(code));
  result->SetStringKey("message", message);

  return std::move(result);
}

std::unique_ptr<base::Value> FormProviderResponse(
    const std::string& controller_response,
    bool* reject) {
  DCHECK(reject);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          controller_response, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& response = value_with_error.value;

  if (!response) {
    ProviderErrors code = ProviderErrors::kUnsupportedMethod;
    std::string message =
        "Invalid response, could not parse JSON: " + controller_response;

    return FormProviderResponse(code, message);
  }

  const base::Value* error = response->FindKey("error");
  if (error) {
    *reject = true;

    return base::Value::ToUniquePtrValue(error->Clone());
  }
  *reject = false;

  // We have a result
  const base::Value* result = response->FindKey("result");
  DCHECK(result);

  return base::Value::ToUniquePtrValue(result->Clone());
}

}  // namespace brave_wallet
