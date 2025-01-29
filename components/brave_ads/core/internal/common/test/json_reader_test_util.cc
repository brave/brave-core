/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/json_reader_test_util.h"

#include <utility>

#include "base/json/json_reader.h"

namespace brave_ads::test {

std::optional<base::Value::Dict> ReadDictionaryFromJsonString(
    const std::string& json) {
  std::optional<base::Value> json_value = base::JSONReader::Read(json);

  if (!json_value) {
    return std::nullopt;
  }

  if (!json_value->is_dict()) {
    return std::nullopt;
  }

  return std::move(*json_value).TakeDict();
}

}  // namespace brave_ads::test
