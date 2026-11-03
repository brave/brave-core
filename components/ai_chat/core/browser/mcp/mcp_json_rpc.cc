/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_json_rpc.h"

#include <utility>

namespace ai_chat {

namespace {

constexpr char kJsonRpcVersion[] = "2.0";

// JSON-RPC envelope keys.
constexpr char kJsonRpcKey[] = "jsonrpc";
constexpr char kIdKey[] = "id";
constexpr char kMethodKey[] = "method";
constexpr char kParamsKey[] = "params";
constexpr char kResultKey[] = "result";
constexpr char kErrorKey[] = "error";
constexpr char kCodeKey[] = "code";
constexpr char kMessageKey[] = "message";

std::optional<JsonRpcId> ParseId(const base::Value& id) {
  if (id.is_int()) {
    return id.GetInt();
  }
  if (id.is_string()) {
    return id.GetString();
  }
  return std::nullopt;
}

base::Value IdToValue(const JsonRpcId& id) {
  return std::visit([](const auto& value) { return base::Value(value); }, id);
}

base::DictValue MakeJsonRpcMessage() {
  return base::DictValue().Set(kJsonRpcKey, kJsonRpcVersion);
}

}  // namespace

JsonRpcNotification::JsonRpcNotification() = default;
JsonRpcNotification::JsonRpcNotification(JsonRpcNotification&&) noexcept =
    default;
JsonRpcNotification& JsonRpcNotification::operator=(
    JsonRpcNotification&&) noexcept = default;
JsonRpcNotification::~JsonRpcNotification() = default;

JsonRpcResponse::JsonRpcResponse() = default;
JsonRpcResponse::JsonRpcResponse(JsonRpcResponse&&) noexcept = default;
JsonRpcResponse& JsonRpcResponse::operator=(JsonRpcResponse&&) noexcept =
    default;
JsonRpcResponse::~JsonRpcResponse() = default;

base::DictValue MakeJsonRpcRequest(int id,
                                   std::string_view method,
                                   std::optional<base::DictValue> params) {
  base::DictValue message = MakeJsonRpcMessage();
  message.Set(kIdKey, id);
  message.Set(kMethodKey, method);
  if (params) {
    message.Set(kParamsKey, std::move(*params));
  }
  return message;
}

base::DictValue MakeJsonRpcNotification(std::string_view method,
                                        std::optional<base::DictValue> params) {
  base::DictValue message = MakeJsonRpcMessage();
  message.Set(kMethodKey, method);
  if (params) {
    message.Set(kParamsKey, std::move(*params));
  }
  return message;
}

base::DictValue MakeJsonRpcResult(const JsonRpcId& id, base::DictValue result) {
  base::DictValue message = MakeJsonRpcMessage();
  message.Set(kIdKey, IdToValue(id));
  message.Set(kResultKey, std::move(result));
  return message;
}

base::DictValue MakeJsonRpcErrorResponse(const JsonRpcId& id,
                                         int code,
                                         std::string_view message_text) {
  base::DictValue message = MakeJsonRpcMessage();
  message.Set(kIdKey, IdToValue(id));
  message.Set(
      kErrorKey,
      base::DictValue().Set(kCodeKey, code).Set(kMessageKey, message_text));
  return message;
}

std::optional<JsonRpcMessage> ParseJsonRpcMessage(base::DictValue message) {
  const std::string* version = message.FindString(kJsonRpcKey);
  if (!version || *version != kJsonRpcVersion) {
    return std::nullopt;
  }
  const base::Value* id = message.Find(kIdKey);

  if (const base::Value* method = message.Find(kMethodKey)) {
    if (!method->is_string()) {
      return std::nullopt;
    }
    if (id) {
      std::optional<JsonRpcId> request_id = ParseId(*id);
      if (!request_id) {
        return std::nullopt;
      }
      return JsonRpcRequest{std::move(*request_id), method->GetString()};
    }
    JsonRpcNotification notification;
    notification.method = method->GetString();
    if (base::Value* params = message.Find(kParamsKey)) {
      if (!params->is_dict()) {
        return std::nullopt;
      }
      notification.params = std::move(params->GetDict());
    }
    return notification;
  }

  base::Value* result = message.Find(kResultKey);
  base::Value* error = message.Find(kErrorKey);
  if (!result == !error) {
    return std::nullopt;
  }
  JsonRpcResponse response;
  // Only an error response may have a null id.
  if (id && !id->is_none()) {
    response.id = ParseId(*id);
    if (!response.id) {
      return std::nullopt;
    }
  }
  if (result) {
    if (!response.id || !result->is_dict()) {
      return std::nullopt;
    }
    response.result = std::move(result->GetDict());
    return response;
  }
  if (!error->is_dict()) {
    return std::nullopt;
  }
  std::optional<int> code = error->GetDict().FindInt(kCodeKey);
  std::string* error_message = error->GetDict().FindString(kMessageKey);
  if (!code || !error_message) {
    return std::nullopt;
  }
  response.result =
      base::unexpected(JsonRpcError{*code, std::move(*error_message)});
  return response;
}

bool IsJsonRpcResponseShaped(const base::DictValue& message) {
  return !message.contains(kMethodKey) &&
         (message.contains(kResultKey) || message.contains(kErrorKey));
}

}  // namespace ai_chat
