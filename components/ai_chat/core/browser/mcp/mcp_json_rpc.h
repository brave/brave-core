/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_JSON_RPC_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_JSON_RPC_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "base/types/expected.h"
#include "base/values.h"

namespace ai_chat {

// JSON-RPC 2.0, restricted to what MCP allows: ids are integers or strings
// and results are objects.
using JsonRpcId = std::variant<int, std::string>;

struct JsonRpcError {
  bool operator==(const JsonRpcError&) const = default;

  int code = 0;
  std::string message;
};

struct JsonRpcRequest {
  JsonRpcId id;
  std::string method;
};

struct JsonRpcNotification {
  JsonRpcNotification();
  JsonRpcNotification(JsonRpcNotification&&) noexcept;
  JsonRpcNotification& operator=(JsonRpcNotification&&) noexcept;
  ~JsonRpcNotification();

  std::string method;
  std::optional<base::DictValue> params;
};

struct JsonRpcResponse {
  JsonRpcResponse();
  JsonRpcResponse(JsonRpcResponse&&) noexcept;
  JsonRpcResponse& operator=(JsonRpcResponse&&) noexcept;
  ~JsonRpcResponse();

  // Unset only for an error the server couldn't attribute to a request.
  std::optional<JsonRpcId> id;
  base::expected<base::DictValue, JsonRpcError> result;
};

using JsonRpcMessage =
    std::variant<JsonRpcRequest, JsonRpcNotification, JsonRpcResponse>;

inline constexpr int kJsonRpcMethodNotFound = -32601;

base::DictValue MakeJsonRpcRequest(int id,
                                   std::string_view method,
                                   std::optional<base::DictValue> params);
base::DictValue MakeJsonRpcNotification(std::string_view method,
                                        std::optional<base::DictValue> params);
base::DictValue MakeJsonRpcResult(const JsonRpcId& id, base::DictValue result);
base::DictValue MakeJsonRpcErrorResponse(const JsonRpcId& id,
                                         int code,
                                         std::string_view message);

// Returns nullopt for anything that isn't a well-formed JSON-RPC 2.0 message.
// Unknown fields are ignored.
std::optional<JsonRpcMessage> ParseJsonRpcMessage(base::DictValue message);

// Whether `message` is shaped like a response, well-formed or not: it has a
// `result` or an `error`, and no `method`.
bool IsJsonRpcResponseShaped(const base::DictValue& message);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_JSON_RPC_H_
