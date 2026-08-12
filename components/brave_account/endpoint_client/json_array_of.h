/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_JSON_ARRAY_OF_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_JSON_ARRAY_OF_H_

#include <optional>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_response_body.h"

namespace brave_account::endpoint_client {

// Response body for endpoints returning a top-level JSON array.
//
// The schema compiler cannot generate one: IDL has no syntax for it, and a
// JSON schema yields only a `using X = std::vector<T>;` alias, which has no
// FromValue() and so cannot satisfy IsJSONResponseBody.
template <detail::IsJSONResponseBody T>
struct JSONArrayOf {
  static std::optional<JSONArrayOf<T>> FromValue(const base::Value& value) {
    const base::ListValue* list = value.GetIfList();
    if (!list) {
      return std::nullopt;
    }

    JSONArrayOf<T> array;
    array.items.reserve(list->size());

    for (const base::Value& item : *list) {
      // A single unparseable element fails the whole array.
      std::optional<T> parsed = T::FromValue(item);
      if (!parsed) {
        return std::nullopt;
      }

      array.items.push_back(*std::move(parsed));
    }

    return array;
  }

  std::vector<T> items;
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_JSON_ARRAY_OF_H_
