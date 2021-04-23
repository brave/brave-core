/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

namespace brave_wallet {

std::string FormProviderResponse(ProviderErrors code,
                                 const std::string& message) {
  std::string formed_response;
  base::DictionaryValue json_root;
  json_root.SetIntKey("code", static_cast<int>(code));
  json_root.SetStringKey("message", message);
  base::JSONWriter::Write(json_root, &formed_response);

  return formed_response;
}

std::string FormProviderResponse(const std::string& controller_response,
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

  std::string formed_response;
  // Check, do we have any error message
  const base::Value* error = response->FindKey("error");
  if (error) {
    base::JSONWriter::Write(*error, &formed_response);
    *reject = true;

    return formed_response;
  }
  *reject = false;

  // We have a result
  const base::Value* result = response->FindKey("result");
  DCHECK(result);
  if (result)
    base::JSONWriter::Write(*result, &formed_response);

  return formed_response;
}

}  // namespace brave_wallet
