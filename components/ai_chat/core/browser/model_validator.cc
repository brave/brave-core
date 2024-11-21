/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/model_validator.h"

#include <string>

#include "base/numerics/safe_math.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/net/base/url_util.h"
#include "net/base/ip_address.h"

class GURL;

namespace ai_chat {

namespace {
bool IsValidPrivateIPAddress(const GURL& endpoint) {
  net::IPAddress ip_address;
  // Extract the host
  std::string host = endpoint.host();

  // Parse the hostname to an IPAddress
  if (!net::ParseURLHostnameToAddress(host, &ip_address) ||
      !ip_address.IsValid()) {
    return false;
  }

  // Allow loopback addresses
  if (ip_address.IsLoopback()) {
    return true;
  }

  // Allow link-local addresses
  if (ip_address.IsLinkLocal()) {
    return true;
  }

  // Allow unique local IPv6 addresses
  if (ip_address.IsUniqueLocalIPv6()) {
    return true;
  }

  // Allow private IPv4 addresses
  if (ip_address.IsIPv4() && !ip_address.IsPubliclyRoutable()) {
    return true;
  }

  // IP address is not in allowed ranges
  return false;
}

}  // namespace
// Static
bool ModelValidator::IsValidContextSize(const std::optional<int32_t>& size) {
  if (!size.has_value()) {
    return false;
  }

  base::CheckedNumeric<size_t> checked_value(size.value());

  return checked_value.IsValid() &&
         checked_value.ValueOrDie() >= kMinCustomModelContextSize &&
         checked_value.ValueOrDie() <= kMaxCustomModelContextSize;
}

// Static
bool ModelValidator::HasValidContextSize(
    const mojom::CustomModelOptions& options) {
  return IsValidContextSize(options.context_size);
}

// Static
bool ModelValidator::IsValidEndpoint(const GURL& endpoint,
                                     std::optional<bool> check_as_private_ip) {
  // HTTPS and localhost URLs are always allowed.
  if (net::IsHTTPSOrLocalhostURL(endpoint)) {
    return true;
  }

  // The following condition is only met when `true` is passed as
  // check_as_private_ip or when the optional feature is enabled. Intentionally,
  // it will not be met when `false` is passed as check_as_private_ip.
  if (check_as_private_ip.value_or(
          ai_chat::features::IsAllowPrivateIPsEnabled())) {
    if (IsValidPrivateIPAddress(endpoint)) {
      VLOG(2) << "Allowing private endpoint: " << endpoint.spec();
      return true;
    }
  }

  return false;
}

ModelValidationResult ModelValidator::ValidateCustomModelOptions(
    const mojom::CustomModelOptions& options) {
  if (!HasValidContextSize(options)) {
    return ModelValidationResult::kInvalidContextSize;
  }

  if (!IsValidEndpoint(options.endpoint)) {
    return ModelValidationResult::kInvalidUrl;
  }

  // Add further validations as needed
  return ModelValidationResult::kSuccess;
}

}  // namespace ai_chat
