// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/json/json_converter_impl.h"

#include "brave/components/json/rs/src/lib.rs.h"

namespace brave {
namespace json {

JsonConverterImpl::JsonConverterImpl(
    mojo::PendingReceiver<mojom::JsonConverter> receiver)
    : receiver_(this, std::move(receiver)) {}
JsonConverterImpl::~JsonConverterImpl() = default;

void JsonConverterImpl::ConvertAllNumbersToString(
    const std::string& json,
    const std::string& path,
    ConvertAllNumbersToStringCallback callback) {
  auto converted_json =
      std::string(::json::convert_all_numbers_to_string(json, path));
  std::move(callback).Run(converted_json);
}

}  // namespace json
}  // namespace brave
