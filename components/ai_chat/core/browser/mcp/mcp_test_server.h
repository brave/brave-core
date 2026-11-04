/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TEST_SERVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TEST_SERVER_H_

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/values.h"

namespace network {
struct ResourceRequest;
class TestURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Answers, with JSON, the requests that MCP clients send through a
// TestURLLoaderFactory. Requests are indexed in the order they were sent,
// including those already answered.
class MCPTestServer {
 public:
  explicit MCPTestServer(network::TestURLLoaderFactory& url_loader_factory);
  MCPTestServer(const MCPTestServer&) = delete;
  MCPTestServer& operator=(const MCPTestServer&) = delete;
  ~MCPTestServer();

  // Runs tasks until request `index` has been sent.
  void WaitForRequest(size_t index);

  size_t RequestCount() const;
  // Request `index` must have been sent.
  const network::ResourceRequest& Request(size_t index) const;
  base::DictValue Body(size_t index) const;
  // The JSON-RPC method of request `index`, if it has one.
  std::string Method(size_t index) const;

  // Answers request `index` with JSON-RPC result `result_json`.
  void RespondResult(size_t index, std::string_view result_json);
  // Answers request `index` with a JSON-RPC error.
  void RespondError(size_t index, int code, std::string_view message);
  // Answers notification `index`.
  void RespondAccepted(size_t index);
  // Waits for and answers the `initialize` at `index`, issuing `session_id` if
  // it's set, and the `notifications/initialized` after it. Returns the index
  // of the request that follows.
  size_t CompleteHandshake(
      size_t index,
      std::optional<std::string_view> session_id = std::nullopt);

 private:
  // A JSON-RPC response to request `index`, with result `result_json`.
  std::string ResultMessage(size_t index, std::string_view result_json) const;
  void Respond(size_t index,
               int status,
               std::string_view body,
               std::optional<std::string_view> session_id = std::nullopt);
  void OnRequest(const network::ResourceRequest& request);

  const raw_ref<network::TestURLLoaderFactory> url_loader_factory_;
  // Quits the WaitForRequest() that's running, if any.
  base::RepeatingClosure quit_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TEST_SERVER_H_
