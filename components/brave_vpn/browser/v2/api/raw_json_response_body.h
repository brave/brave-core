/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_RAW_JSON_RESPONSE_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_RAW_JSON_RESPONSE_BODY_H_

#include <optional>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

namespace brave_vpn::v2::endpoints {

// A response body that captures the server's JSON payload verbatim
// (re-serialized) instead of mapping it to typed fields. For endpoints whose
// response is opaque to the browser and forwarded elsewhere for interpretation.
struct RawJsonResponseBody {
  std::string json;

  static std::optional<RawJsonResponseBody> FromValue(
      const base::Value& value) {
    return base::WriteJson(value).transform([](std::string json) {
      return RawJsonResponseBody{.json = std::move(json)};
    });
  }

  base::DictValue ToValue() const {
    return CHECK_DEREF(base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC));
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_RAW_JSON_RESPONSE_BODY_H_
