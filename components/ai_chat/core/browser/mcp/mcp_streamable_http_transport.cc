/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_streamable_http_transport.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/origin.h"

namespace ai_chat {

namespace {

constexpr char kJsonMimeType[] = "application/json";
constexpr char kEventStreamMimeType[] = "text/event-stream";
// Both types are required by the Streamable HTTP spec.
constexpr char kAcceptValue[] = "application/json, text/event-stream";

// A detached request outlives the transport, so bound how long it lingers.
constexpr base::TimeDelta kDetachedRequestTimeout = base::Seconds(10);
// The reply to a detached request is ignored, so don't buffer much of it.
constexpr size_t kDetachedRequestMaxResponseBytes = 16 * 1024;

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("ai_chat_mcp", R"(
      semantics {
        sender: "AI Chat"
        description:
          "Communicates with a Model Context Protocol (MCP) server that the "
          "user has configured, to list the tools it provides and to call "
          "them on the user's behalf."
        trigger:
          "AI Chat needs the tools of a user-configured MCP server, or the "
          "assistant calls one of them."
        data:
          "MCP JSON-RPC messages, including the names and arguments of tool "
          "calls, which can contain conversation content and PII. Includes "
          "the access token for the server if the user provided one."
        destination: OTHER
        destination_other: "An MCP server configured by the user."
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification: "Not implemented."
      })");

MCPTransport::Error ErrorFromResult(
    const api_request_helper::APIRequestResult& result,
    const GURL& endpoint) {
  switch (result.error_code()) {
    case net::ERR_TIMED_OUT:
      return {MCPErrorCode::kTimeout};
    // How `APIRequestHelper` reports exceeding `max_body_size`.
    case net::ERR_INSUFFICIENT_RESOURCES:
      return {MCPErrorCode::kResponseTooLarge};
    case net::ERR_UNSAFE_REDIRECT:
      // `APIRequestHelper` refused a redirect to `final_url()`. Otherwise net
      // refused one to an unsupported scheme, leaving `final_url()` unchanged.
      if (!url::IsSameOriginWith(result.final_url(), endpoint)) {
        return {.code = MCPErrorCode::kCrossOriginRedirect,
                .redirect_url = result.final_url()};
      }
      return {MCPErrorCode::kNetworkError, result.error_code()};
    default:
      return {MCPErrorCode::kNetworkError, result.error_code()};
  }
}

// Redirects to another origin would also carry the access token.
api_request_helper::APIRequestOptions MakeRequestOptions(
    size_t max_body_size,
    base::TimeDelta timeout) {
  api_request_helper::APIRequestOptions options;
  options.max_body_size = max_body_size;
  options.timeout = timeout;
  options.same_origin_redirects_only = true;
  return options;
}

}  // namespace

MCPStreamableHttpTransport::PendingRequest::PendingRequest() = default;
MCPStreamableHttpTransport::PendingRequest::PendingRequest(
    PendingRequest&&) noexcept = default;
MCPStreamableHttpTransport::PendingRequest&
MCPStreamableHttpTransport::PendingRequest::operator=(
    PendingRequest&&) noexcept = default;
MCPStreamableHttpTransport::PendingRequest::~PendingRequest() = default;

MCPStreamableHttpTransport::MCPStreamableHttpTransport(
    GURL endpoint,
    std::optional<std::string> bearer_token,
    size_t max_response_bytes,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : endpoint_(std::move(endpoint)),
      bearer_token_(std::move(bearer_token)),
      max_response_bytes_(max_response_bytes),
      url_loader_factory_(url_loader_factory),
      api_request_helper_(kTrafficAnnotation, std::move(url_loader_factory)) {
  CHECK(!bearer_token_ || net::HttpUtil::IsValidHeaderValue(*bearer_token_));
}

MCPStreamableHttpTransport::~MCPStreamableHttpTransport() = default;

MCPTransport::RequestId MCPStreamableHttpTransport::Send(
    const base::DictValue& message,
    const Headers& headers,
    base::TimeDelta timeout,
    MessageCallback on_message,
    CompleteCallback on_complete) {
  const RequestId id = ++next_request_id_;
  PendingRequest& request = requests_[id];
  request.on_message = std::move(on_message);
  request.on_complete = std::move(on_complete);

  std::optional<std::string> payload = base::WriteJson(message);
  if (!payload) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&MCPStreamableHttpTransport::Complete,
                       weak_ptr_factory_.GetWeakPtr(), id,
                       base::expected<Response, Error>(base::unexpected(
                           Error{MCPErrorCode::kInvalidRequest}))));
    return id;
  }

  request.ticket = api_request_helper_.RequestSSE(
      net::HttpRequestHeaders::kPostMethod, endpoint_, *payload, kJsonMimeType,
      base::BindRepeating(&MCPStreamableHttpTransport::OnData,
                          weak_ptr_factory_.GetWeakPtr(), id),
      base::BindOnce(&MCPStreamableHttpTransport::OnResult,
                     weak_ptr_factory_.GetWeakPtr(), id),
      MakeHeaders(headers), MakeRequestOptions(max_response_bytes_, timeout),
      base::BindOnce(&MCPStreamableHttpTransport::OnResponseStarted,
                     weak_ptr_factory_.GetWeakPtr(), id));
  return id;
}

void MCPStreamableHttpTransport::Cancel(RequestId id) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  api_request_helper_.Cancel(it->second.ticket);
  requests_.erase(it);
}

void MCPStreamableHttpTransport::SendDetached(const base::DictValue& message,
                                              const Headers& headers) {
  std::optional<std::string> payload = base::WriteJson(message);
  if (!payload) {
    return;
  }
  StartDetachedRequest(net::HttpRequestHeaders::kPostMethod,
                       std::move(*payload), headers);
}

void MCPStreamableHttpTransport::TerminateSession(const Headers& headers) {
  // A 405 reply means the server doesn't let clients end sessions.
  StartDetachedRequest(net::HttpRequestHeaders::kDeleteMethod, std::string(),
                       headers);
}

void MCPStreamableHttpTransport::StartDetachedRequest(std::string_view method,
                                                      std::string payload,
                                                      const Headers& headers) {
  // Owned by its own result callback, so the request survives this transport.
  auto helper = std::make_unique<api_request_helper::APIRequestHelper>(
      kTrafficAnnotation, url_loader_factory_);
  api_request_helper::APIRequestHelper* helper_ptr = helper.get();
  const std::string content_type = payload.empty() ? "" : kJsonMimeType;
  helper_ptr->Request(
      std::string(method), endpoint_, payload, content_type,
      base::BindOnce(
          [](std::unique_ptr<api_request_helper::APIRequestHelper>,
             api_request_helper::APIRequestResult result) {
            DVLOG(1) << "Detached MCP request status " << result.response_code()
                     << ", error " << result.error_code();
          },
          std::move(helper)),
      MakeHeaders(headers),
      MakeRequestOptions(kDetachedRequestMaxResponseBytes,
                         kDetachedRequestTimeout));
}

MCPTransport::Headers MCPStreamableHttpTransport::MakeHeaders(
    const Headers& headers) const {
  Headers result = headers;
  result[net::HttpRequestHeaders::kAccept] = kAcceptValue;
  if (bearer_token_) {
    result[net::HttpRequestHeaders::kAuthorization] =
        base::StrCat({"Bearer ", *bearer_token_});
  }
  return result;
}

void MCPStreamableHttpTransport::OnResponseStarted(
    RequestId id,
    const api_request_helper::ResponseStartedInfo& info) {
  auto it = requests_.find(id);
  if (it != requests_.end()) {
    it->second.mime_type = info.mime_type;
    it->second.response_headers = info.headers;
  }
}

void MCPStreamableHttpTransport::OnData(RequestId id,
                                        api_request_helper::ValueOrError data) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  PendingRequest& request = it->second;
  if (request.mime_type == kEventStreamMimeType) {
    // Each SSE event arrives parsed.
    if (!data.has_value() || !data->is_dict()) {
      DVLOG(1) << "Ignoring an MCP event that isn't a JSON object";
      return;
    }
    // Copied, because the callback may destroy `this`.
    MessageCallback on_message = request.on_message;
    const Headers response_headers = request.response_headers;
    on_message.Run(std::move(*data).TakeDict(), response_headers);
    return;
  }
  // Any other body arrives as raw chunks, bounded by `max_body_size`.
  if (!data.has_value() || !data->is_string()) {
    return;
  }
  if (request.mime_type == kJsonMimeType) {
    request.json_body.append(data->GetString());
  } else if (!data->GetString().empty()) {
    request.has_other_body = true;
  }
}

void MCPStreamableHttpTransport::OnResult(
    RequestId id,
    api_request_helper::APIRequestResult result) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  // Checked first, because an aborted stream can still report a 2xx status.
  if (result.error_code() != net::OK) {
    Complete(id, base::unexpected(ErrorFromResult(result, endpoint_)));
    return;
  }
  if (!result.IsResponseCodeValid()) {
    Complete(id, base::unexpected(Error{MCPErrorCode::kNetworkError,
                                        net::ERR_INVALID_RESPONSE}));
    return;
  }

  Response response;
  response.status_code = result.response_code();
  response.headers = result.headers();
  if (!result.Is2XXResponseCode()) {
    // `APIRequestHelper` parses a non-2xx JSON body into the result.
    if (result.value_body().is_dict()) {
      response.error_body = std::move(result).TakeBody().TakeDict();
    }
    Complete(id, std::move(response));
    return;
  }

  PendingRequest& request = it->second;
  if (request.has_other_body) {
    Complete(id, base::unexpected(Error{MCPErrorCode::kInvalidResponse}));
    return;
  }
  if (request.json_body.empty()) {
    // An SSE stream, whose messages have all been delivered, or no body.
    Complete(id, std::move(response));
    return;
  }
  api_request_helper::ParseJsonNonBlocking(
      std::move(request.json_body),
      base::BindOnce(&MCPStreamableHttpTransport::OnBodyParsed,
                     weak_ptr_factory_.GetWeakPtr(), id, std::move(response)));
}

void MCPStreamableHttpTransport::OnBodyParsed(
    RequestId id,
    Response response,
    api_request_helper::ValueOrError body) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  if (!body.has_value() || !body->is_dict()) {
    Complete(id, base::unexpected(Error{MCPErrorCode::kInvalidResponse}));
    return;
  }
  // Copied, because the callback may destroy `this` or cancel the request.
  MessageCallback on_message = it->second.on_message;
  base::WeakPtr<MCPStreamableHttpTransport> weak_this =
      weak_ptr_factory_.GetWeakPtr();
  on_message.Run(std::move(*body).TakeDict(), response.headers);
  if (!weak_this) {
    return;
  }
  Complete(id, std::move(response));
}

void MCPStreamableHttpTransport::Complete(
    RequestId id,
    base::expected<Response, Error> result) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  CompleteCallback on_complete = std::move(it->second.on_complete);
  requests_.erase(it);
  std::move(on_complete).Run(std::move(result));
}

}  // namespace ai_chat
