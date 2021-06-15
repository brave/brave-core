/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

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
    const bool send_async,
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

  if (send_async) {
    return base::Value::ToUniquePtrValue(response->Clone());
  }

  // We have a result
  const base::Value* result = response->FindKey("result");
  DCHECK(result);

  return base::Value::ToUniquePtrValue(result->Clone());
}

std::string FormProviderErrorResponse(const std::string& controller_response) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          controller_response, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& response = value_with_error.value;

  if (response) {
    const base::Value* error = response->FindKey("error");
    if (error) {
      std::string error_response;
      if (!base::JSONWriter::Write(*error, &error_response))
        return "";

      return error_response;
    }
  }

  ProviderErrors code = ProviderErrors::kUnsupportedMethod;
  std::string message =
      "Invalid response, could not parse JSON: " + controller_response;

  std::string error_response;
  if (!base::JSONWriter::Write(*FormProviderResponse(code, message),
                               &error_response))
    return "";

  return error_response;
}

}  // namespace brave_wallet
