/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_CLIENT_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_transport.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

struct JsonRpcError;
struct JsonRpcRequest;

// Most tool calls finish well within this. Progress notifications restart it.
inline constexpr base::TimeDelta kMCPRequestTimeout = base::Seconds(60);
// A server can't hold a call, and its connection, open for longer than this,
// however often it reports progress.
inline constexpr base::TimeDelta kMCPMaxRequestTimeout = base::Minutes(5);
// Bounds the memory one response from an untrusted server can use. Far more
// than the model can make use of.
inline constexpr size_t kMCPMaxResponseBytes = 4 * 1024 * 1024;
// Bounds the requests a server can cause by always returning a `nextCursor`.
inline constexpr size_t kMCPMaxToolsListPages = 10;
// More tools than this from one server can't usefully be offered to a model.
inline constexpr size_t kMCPMaxToolsPerServer = 100;
// Servers of these protocol versions don't say how long a tool list stays
// valid, so a fixed lifetime bounds both staleness and `tools/list` traffic.
inline constexpr base::TimeDelta kMCPToolsCacheTtl = base::Minutes(5);

// A client for one remote MCP server over Streamable HTTP, speaking protocol
// version 2025-11-25, 2025-06-18 or 2025-03-26. Connects lazily, on the first
// GetTools() or CallTool(), and queues calls behind the handshake. If the
// server expires the session, re-initializes and retries the affected requests
// once. Callbacks never run synchronously or after the client is destroyed.
class MCPClient {
 public:
  enum class State {
    // Not connected. Connects on the next GetTools() or CallTool().
    kIdle,
    kInitializing,
    kReady,
    // Connecting failed, as described by last_error(). Tries again on the
    // next GetTools() or CallTool().
    kFailed,
  };

  using ToolsResult = base::expected<std::vector<MCPToolDefinition>, MCPError>;
  using GetToolsCallback = base::OnceCallback<void(ToolsResult)>;
  using CallToolCallback =
      base::OnceCallback<void(base::expected<MCPCallToolResult, MCPError>)>;

  // A client of the Streamable HTTP server `config` describes, or the reason
  // `config` can't be used.
  static base::expected<std::unique_ptr<MCPClient>, MCPError> Create(
      MCPServerConfig config,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // `display_name` names the server in errors. `has_bearer_token` is whether
  // `transport` authenticates its requests.
  MCPClient(std::string display_name,
            bool has_bearer_token,
            std::unique_ptr<MCPTransport> transport);
  MCPClient(const MCPClient&) = delete;
  MCPClient& operator=(const MCPClient&) = delete;
  // Tells the server to stop work on calls in flight and ends the session,
  // best effort.
  ~MCPClient();

  // Lists the server's tools, at most kMCPMaxToolsPerServer of them. Served
  // from a cache for kMCPToolsCacheTtl.
  void GetTools(GetToolsCallback callback);

  // Calls tool `name`. Running the returned closure cancels the call: the
  // server is notified and `callback` won't run. The closure does nothing
  // once `callback` has run.
  [[nodiscard]] base::OnceClosure CallTool(std::string_view name,
                                           base::DictValue arguments,
                                           CallToolCallback callback);

  // Ends the session, best effort, so that the next request starts another
  // with `initialize`. Calls in flight carry on, and the server is told once
  // they've finished.
  void EndSession();

  State state() const { return state_; }
  // Why connecting failed, while state() is kFailed.
  const std::optional<MCPError>& last_error() const { return last_error_; }

  base::WeakPtr<MCPClient> GetWeakPtr();

 private:
  // `response_headers` are those of the response that carried the result.
  using ResultCallback =
      base::OnceCallback<void(base::expected<base::DictValue, MCPError> result,
                              const MCPTransport::Headers& response_headers)>;
  using TransportResult =
      base::expected<MCPTransport::Response, MCPTransport::Error>;

  struct Request;

  // Sends `method` now if it's `initialize` or the client is ready, and
  // otherwise once the handshake completes.
  void SendRequest(int id,
                   std::string_view method,
                   std::optional<base::DictValue> params,
                   ResultCallback callback);
  void ScheduleRequest(int id);
  void Dispatch(int id);
  void StartTimeout(Request& request, int id);
  void OnMessage(int sent_request_id,
                 base::DictValue message,
                 const MCPTransport::Headers& response_headers);
  void MarkInvalidResponse(int id);
  void OnRequestComplete(int id, TransportResult result);
  void OnRequestTimeout(int id);
  void CompleteRequest(int id,
                       base::expected<base::DictValue, MCPError> result,
                       const MCPTransport::Headers& response_headers = {});
  void CancelRequest(int id);
  void SendCancelledNotification(int id,
                                 const MCPTransport::Headers& headers,
                                 std::string_view reason);

  // Tells the server of each ended session that has no calls left in flight.
  void TerminateEndedSessions();

  void SendNotification(std::string_view method,
                        std::optional<base::DictValue> params,
                        MCPTransport::CompleteCallback on_complete);
  void ReplyToServerRequest(const JsonRpcRequest& request,
                            const MCPTransport::Headers& headers);
  MCPTransport::Headers SessionHeaders() const;
  void ResetSession();

  void StartInitialize();
  void OnInitializeResponse(base::expected<base::DictValue, MCPError> result,
                            const MCPTransport::Headers& response_headers);
  void OnInitializedNotificationComplete(TransportResult result);
  void FailInitialize(MCPError error);

  void RequestToolsPage(std::optional<std::string> cursor);
  void OnToolsListPage(base::expected<base::DictValue, MCPError> result,
                       const MCPTransport::Headers& response_headers);
  void FinishGetTools(ToolsResult result);
  void RunGetToolsCallback(GetToolsCallback callback,
                           std::vector<MCPToolDefinition> tools);
  void OnCallToolResponse(CallToolCallback callback,
                          base::expected<base::DictValue, MCPError> result,
                          const MCPTransport::Headers& response_headers);

  MCPError MakeError(MCPErrorCode code) const;
  MCPError MakeTransportError(const MCPTransport::Error& error) const;
  MCPError MakeHttpError(const MCPTransport::Response& response,
                         bool is_initialize) const;
  MCPError MakeServerError(const JsonRpcError& error) const;

  const std::string display_name_;
  const bool has_bearer_token_;
  const std::unique_ptr<MCPTransport> transport_;

  State state_ = State::kIdle;
  std::optional<MCPError> last_error_;

  // Session state, in memory only. The session id is a secret: never log it.
  std::optional<std::string> protocol_version_;
  std::optional<std::string> session_id_;
  // Incremented whenever the session is discarded, so that the expiry of a
  // session that's already been replaced isn't acted on twice.
  int session_generation_ = 0;
  // Sessions that EndSession() ended while calls in them were in flight.
  struct EndedSession {
    int generation = 0;
    MCPTransport::Headers headers;
  };
  std::vector<EndedSession> ended_sessions_;

  int next_request_id_ = 0;
  absl::flat_hash_map<int, std::unique_ptr<Request>> requests_;
  // Requests waiting for the handshake.
  std::vector<int> queued_requests_;

  std::vector<GetToolsCallback> get_tools_callbacks_;
  std::vector<MCPToolDefinition> tools_in_progress_;
  size_t tools_pages_fetched_ = 0;
  std::optional<std::vector<MCPToolDefinition>> tools_cache_;
  base::TimeTicks tools_cache_time_;

  base::WeakPtrFactory<MCPClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_CLIENT_H_
