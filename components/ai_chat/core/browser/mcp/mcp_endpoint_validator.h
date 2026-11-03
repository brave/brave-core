/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_ENDPOINT_VALIDATOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_ENDPOINT_VALIDATOR_H_

class GURL;

namespace ai_chat {

enum class MCPEndpointValidationResult {
  kValid,
  // Also an unspecified address, 0.0.0.0 or [::].
  kInvalidUrl,
  // Neither http nor https.
  kUnsupportedScheme,
  // The URL has a username or password.
  kEmbeddedCredentials,
  // http to a host other than localhost, 127.0.0.1 or [::1].
  kInsecureScheme,
  // Another IP address that isn't publicly routable, such as a private or
  // loopback one, while `features::kAllowPrivateIPs` is disabled.
  kPrivateAddressNotAllowed,
};

// Checks whether `endpoint` may be used as an MCP server. https is required,
// except for http to localhost, 127.0.0.1 and [::1]. Other IP addresses that
// aren't publicly routable are only allowed, over http or https, when
// `features::kAllowPrivateIPs` is enabled. Unspecified addresses, 0.0.0.0 and
// [::], are never allowed.
// Stricter than `ModelValidator::IsValidEndpoint()`, since a bearer token may
// be sent to the endpoint.
MCPEndpointValidationResult ValidateMCPEndpoint(const GURL& endpoint);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_ENDPOINT_VALIDATOR_H_
