/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TRANSPORT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TRANSPORT_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "url/gurl.h"

namespace ai_chat {

// Carries JSON-RPC messages to and from one MCP server. Protocol state, such
// as the session and the negotiated version, belongs to the caller, which
// passes whatever headers it needs with each message.
class MCPTransport {
 public:
  using Headers = base::flat_map<std::string, std::string>;
  using RequestId = uint64_t;

  struct Response {
    Response();
    Response(Response&&) noexcept;
    Response& operator=(Response&&) noexcept;
    ~Response();

    int status_code = 0;
    // Names are lowercase.
    Headers headers;
    // The parsed JSON object body of a non-2xx response, if it had one.
    std::optional<base::DictValue> error_body;
  };

  struct Error {
    bool operator==(const Error&) const = default;

    // One of kInvalidRequest, kNetworkError, kTimeout, kResponseTooLarge,
    // kCrossOriginRedirect or kInvalidResponse.
    MCPErrorCode code = MCPErrorCode::kNetworkError;
    // The `net::Error` for kNetworkError.
    int net_error = 0;
    // Where a kCrossOriginRedirect pointed.
    GURL redirect_url;
  };

  // Called for each JSON-RPC message in a 2xx reply, or in any streamed reply,
  // with the headers of the response that carried it. Names are lowercase.
  using MessageCallback =
      base::RepeatingCallback<void(base::DictValue message,
                                   const Headers& response_headers)>;
  using CompleteCallback =
      base::OnceCallback<void(base::expected<Response, Error>)>;

  virtual ~MCPTransport() = default;

  // Sends `message`. `on_message` may run any number of times and is followed
  // by one `on_complete`. Neither runs synchronously, after `Cancel(id)`, or
  // after this transport is destroyed, and either may destroy the transport.
  virtual RequestId Send(const base::DictValue& message,
                         const Headers& headers,
                         base::TimeDelta timeout,
                         MessageCallback on_message,
                         CompleteCallback on_complete) = 0;

  // Stops the request, dropping its callbacks.
  virtual void Cancel(RequestId id) = 0;

  // Sends `message`, which expects no reply, such as a notification. Best
  // effort: the outcome isn't reported, and it may outlive this transport.
  virtual void SendDetached(const base::DictValue& message,
                            const Headers& headers) = 0;

  // Ends the server-side session identified by `headers`. Best effort, like
  // SendDetached().
  virtual void TerminateSession(const Headers& headers) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_TRANSPORT_H_
