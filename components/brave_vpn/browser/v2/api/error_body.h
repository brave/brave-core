/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_ERROR_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_ERROR_BODY_H_

#include <optional>
#include <string>

#include "base/strings/strcat.h"
#include "base/values.h"

namespace brave_vpn::v2::endpoints {

// Error body for Guardian VPN endpoints. Per Guardian's API docs, every non-200
// and non-201 response from both the Connect API and the SGW (per-node) API
// uses the same shape, so one type covers every endpoint group.
struct VpnErrorBody {
  std::string error_title;
  std::string error_message;

  static std::optional<VpnErrorBody> FromValue(const base::Value& value) {
    const auto* dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }
    const auto* title = dict->FindString("error-title");
    const auto* message = dict->FindString("error-message");
    if (!title || !message) {
      return std::nullopt;
    }
    return VpnErrorBody{.error_title = *title, .error_message = *message};
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_ERROR_BODY_H_
