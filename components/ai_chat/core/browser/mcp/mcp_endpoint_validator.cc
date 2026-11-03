/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_endpoint_validator.h"

#include "brave/components/ai_chat/core/common/features.h"
#include "net/base/ip_address.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

MCPEndpointValidationResult ValidateMCPEndpoint(const GURL& endpoint) {
  if (!endpoint.is_valid()) {
    return MCPEndpointValidationResult::kInvalidUrl;
  }
  if (!endpoint.SchemeIsHTTPOrHTTPS()) {
    return MCPEndpointValidationResult::kUnsupportedScheme;
  }
  if (endpoint.has_username() || endpoint.has_password()) {
    return MCPEndpointValidationResult::kEmbeddedCredentials;
  }

  net::IPAddress address;
  const bool is_ip_literal =
      net::ParseURLHostnameToAddress(endpoint.host(), &address);
  if (endpoint.host() == "localhost" ||
      (is_ip_literal && (address == net::IPAddress::IPv4Localhost() ||
                         address == net::IPAddress::IPv6Localhost()))) {
    return MCPEndpointValidationResult::kValid;
  }

  // Not a destination, though connecting to it reaches this machine. Checked
  // as IPv4 too, so that a mapped form can't slip through.
  const net::IPAddress unmapped =
      address.IsIPv4MappedIPv6() ? net::ConvertIPv4MappedIPv6ToIPv4(address)
                                 : address;
  if (is_ip_literal && unmapped.IsZero()) {
    return MCPEndpointValidationResult::kInvalidUrl;
  }
  // Covers private, loopback, link-local and other reserved ranges, including
  // their IPv4-mapped IPv6 forms.
  if (is_ip_literal && !address.IsPubliclyRoutable()) {
    return features::IsAllowPrivateIPsEnabled()
               ? MCPEndpointValidationResult::kValid
               : MCPEndpointValidationResult::kPrivateAddressNotAllowed;
  }
  if (!endpoint.SchemeIs(url::kHttpsScheme)) {
    return MCPEndpointValidationResult::kInsecureScheme;
  }
  return MCPEndpointValidationResult::kValid;
}

}  // namespace ai_chat
