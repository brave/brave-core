/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_STREAMABLE_HTTP_TRANSPORT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_STREAMABLE_HTTP_TRANSPORT_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_transport.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// MCP Streamable HTTP transport (2025-03-26 to 2025-11-25): each message is a
// POST to the endpoint, answered with `application/json`, `text/event-stream`,
// or `202 Accepted` with no body. Requests carry no cookies or credentials
// other than the bearer token, and follow only same-origin redirects.
class MCPStreamableHttpTransport : public MCPTransport {
 public:
  // `bearer_token` must be a valid HTTP header value. Responses over
  // `max_response_bytes` fail with kResponseTooLarge.
  MCPStreamableHttpTransport(
      GURL endpoint,
      std::optional<std::string> bearer_token,
      size_t max_response_bytes,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  MCPStreamableHttpTransport(const MCPStreamableHttpTransport&) = delete;
  MCPStreamableHttpTransport& operator=(const MCPStreamableHttpTransport&) =
      delete;
  ~MCPStreamableHttpTransport() override;

  // MCPTransport:
  RequestId Send(const base::DictValue& message,
                 const Headers& headers,
                 base::TimeDelta timeout,
                 MessageCallback on_message,
                 CompleteCallback on_complete) override;
  void Cancel(RequestId id) override;
  void SendDetached(const base::DictValue& message,
                    const Headers& headers) override;
  void TerminateSession(const Headers& headers) override;

 private:
  struct PendingRequest {
    PendingRequest();
    PendingRequest(PendingRequest&&) noexcept;
    PendingRequest& operator=(PendingRequest&&) noexcept;
    ~PendingRequest();

    api_request_helper::APIRequestHelper::Ticket ticket = 0;
    MessageCallback on_message;
    CompleteCallback on_complete;
    std::string mime_type;
    Headers response_headers;
    // Accumulated chunks of an `application/json` body.
    std::string json_body;
    // Whether any body arrived that is neither JSON nor SSE.
    bool has_other_body = false;
  };

  Headers MakeHeaders(const Headers& headers) const;
  void StartDetachedRequest(std::string_view method,
                            std::string payload,
                            const Headers& headers);

  void OnResponseStarted(RequestId id,
                         const api_request_helper::ResponseStartedInfo& info);
  void OnData(RequestId id, api_request_helper::ValueOrError data);
  void OnResult(RequestId id, api_request_helper::APIRequestResult result);
  void OnBodyParsed(RequestId id,
                    Response response,
                    api_request_helper::ValueOrError body);
  void Complete(RequestId id, base::expected<Response, Error> result);

  const GURL endpoint_;
  const std::optional<std::string> bearer_token_;
  const size_t max_response_bytes_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::flat_map<RequestId, PendingRequest> requests_;
  RequestId next_request_id_ = 0;

  base::WeakPtrFactory<MCPStreamableHttpTransport> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_STREAMABLE_HTTP_TRANSPORT_H_
