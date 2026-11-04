/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TYPES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TYPES_H_

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/values.h"
#include "url/gurl.h"

namespace ai_chat {

// A user-configured remote MCP server.
struct MCPServerConfig {
  MCPServerConfig();
  MCPServerConfig(GURL endpoint,
                  std::optional<std::string> bearer_token,
                  std::string display_name);
  MCPServerConfig(const MCPServerConfig&);
  MCPServerConfig& operator=(const MCPServerConfig&);
  MCPServerConfig(MCPServerConfig&&) noexcept;
  MCPServerConfig& operator=(MCPServerConfig&&) noexcept;
  ~MCPServerConfig();

  bool operator==(const MCPServerConfig&) const;

  // The server's Streamable HTTP endpoint.
  GURL endpoint;
  // Sent as `Authorization: Bearer <token>`. A secret: never log it.
  std::optional<std::string> bearer_token;
  // Names the server in error messages.
  std::string display_name;
};

enum class MCPErrorCode {
  // The endpoint fails `ValidateMCPEndpoint()`.
  kInvalidEndpoint,
  // The token can't be sent in an HTTP header.
  kInvalidToken,
  // The request couldn't be encoded, e.g. its arguments nest too deeply.
  kInvalidRequest,
  // The connection failed before an HTTP response arrived.
  kNetworkError,
  kTimeout,
  kResponseTooLarge,
  // The server redirected to another origin, which isn't followed because it
  // would also receive the access token.
  kCrossOriginRedirect,
  // `initialize` was refused without a JSON-RPC body, which is what servers
  // that only speak the deprecated HTTP+SSE transport do.
  kStreamableHttpNotSupported,
  // 401 or 403 when no token is configured.
  kAuthRequired,
  // 401 or 403 when a token is configured.
  kAuthRejected,
  // The server negotiated a protocol version this client doesn't speak.
  kUnsupportedProtocolVersion,
  // The server doesn't offer tools.
  kToolsNotSupported,
  // The session expired again after re-initializing.
  kSessionExpired,
  // The response stream ended without a response to the request.
  kNoResponse,
  // The server sent something that isn't valid MCP.
  kInvalidResponse,
  // The server answered with a JSON-RPC error.
  kServerError,
  // Any other 4xx HTTP status, apart from those that ask to try again later.
  kHttpClientError,
  // Any other non-2xx HTTP status.
  kHttpError,
};

// An error for the UI, which maps `code` to a localized string, and for the
// model, which is given `message`.
struct MCPError {
  bool operator==(const MCPError&) const = default;

  MCPErrorCode code = MCPErrorCode::kNetworkError;
  // English text, naming the server, for the model only.
  std::string message;
};

// MCP tool definition from `tools/list`.
struct MCPToolDefinition {
  MCPToolDefinition();
  MCPToolDefinition(MCPToolDefinition&&) noexcept;
  MCPToolDefinition& operator=(MCPToolDefinition&&) noexcept;
  ~MCPToolDefinition();

  MCPToolDefinition Clone() const;

  std::string name;
  std::optional<std::string> title;
  std::optional<std::string> description;
  // JSON Schema for the tool's arguments. Server-controlled.
  base::DictValue input_schema;
};

struct MCPTextContent {
  bool operator==(const MCPTextContent&) const = default;

  std::string text;
};

struct MCPImageContent {
  bool operator==(const MCPImageContent&) const = default;

  // Base64-encoded image bytes.
  std::string data;
  // Always one that the model can be given.
  std::string mime_type;
};

// An image of a type the model can't be given, kept only so the model can be
// told it was left out.
struct MCPUnsupportedImageContent {
  bool operator==(const MCPUnsupportedImageContent&) const = default;
};

using MCPContent =
    std::variant<MCPTextContent, MCPImageContent, MCPUnsupportedImageContent>;

// The result of `tools/call`. Content other than text and images is
// dropped.
struct MCPCallToolResult {
  MCPCallToolResult();
  MCPCallToolResult(MCPCallToolResult&&) noexcept;
  MCPCallToolResult& operator=(MCPCallToolResult&&) noexcept;
  ~MCPCallToolResult();

  std::vector<MCPContent> content;
  std::optional<base::DictValue> structured_content;
  // The tool itself failed; `content` describes the failure for the model.
  bool is_error = false;
};

// Returns nullopt if `tool` is malformed.
std::optional<MCPToolDefinition> ParseToolDefinition(
    const base::DictValue& tool);

// Returns nullopt if `result` is malformed. Malformed or unsupported content
// items are dropped.
std::optional<MCPCallToolResult> ParseCallToolResult(base::DictValue result);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TYPES_H_
