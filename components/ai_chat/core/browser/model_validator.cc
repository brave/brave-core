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
bool IsValidPrivateHost(const GURL& endpoint) {
  std::string host = endpoint.host();

  // Ensure the hostname is valid (non-empty and doesn't start with a dot)
  if (host.empty() || host.front() == '.') {
    return false;
  }

  std::string lower_host = base::ToLowerASCII(host);

  // Check if the host ends with ".local"
  if (base::EndsWith(lower_host, ".local", base::CompareCase::SENSITIVE)) {
    return true;
  }

  return false;
}

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
                                     bool check_as_private_ip) {
  // HTTPS and localhost URLs are always allowed.
  if (net::IsHTTPSOrLocalhostURL(endpoint)) {
    return true;
  }

  // Scheme should be HTTP or HTTPS
  if (!endpoint.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  // If enabled, we'll permit certain private endpoints.
  if (ai_chat::features::IsAllowPrivateIPsEnabled() || check_as_private_ip) {
    VLOG(2) << "Evaluating private endpoint for model";
    // If the endpoint is a private hostname, we will allow it.
    return IsValidPrivateHost(endpoint) || IsValidPrivateIPAddress(endpoint);
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
