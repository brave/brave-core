/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"

#include <algorithm>
#include <utility>
#include <variant>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/task/sequenced_task_runner.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_endpoint_validator.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_json_rpc.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_streamable_http_transport.h"
#include "brave/components/version_info/version_info.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "url/origin.h"

namespace ai_chat {

namespace {

// Requested in `initialize`. Servers answer with the version they'll speak.
constexpr char kLatestProtocolVersion[] = "2025-11-25";
constexpr auto kSupportedProtocolVersions =
    base::MakeFixedFlatSet<std::string_view>(
        {"2025-03-26", "2025-06-18", "2025-11-25"});

constexpr char kClientName[] = "Brave";

constexpr char kProtocolVersionHeader[] = "MCP-Protocol-Version";
constexpr char kSessionIdHeader[] = "Mcp-Session-Id";
// Response header names from `MCPTransport` are lowercase.
constexpr char kSessionIdResponseHeader[] = "mcp-session-id";

constexpr char kInitializeMethod[] = "initialize";
constexpr char kInitializedNotification[] = "notifications/initialized";
constexpr char kCancelledNotification[] = "notifications/cancelled";
constexpr char kProgressNotification[] = "notifications/progress";
constexpr char kPingMethod[] = "ping";
constexpr char kToolsListMethod[] = "tools/list";
constexpr char kToolsCallMethod[] = "tools/call";

constexpr char kProtocolVersionKey[] = "protocolVersion";
constexpr char kCapabilitiesKey[] = "capabilities";
constexpr char kClientInfoKey[] = "clientInfo";
constexpr char kNameKey[] = "name";
constexpr char kVersionKey[] = "version";
constexpr char kToolsKey[] = "tools";
constexpr char kCursorKey[] = "cursor";
constexpr char kNextCursorKey[] = "nextCursor";
constexpr char kArgumentsKey[] = "arguments";
constexpr char kMetaKey[] = "_meta";
constexpr char kProgressTokenKey[] = "progressToken";
constexpr char kRequestIdKey[] = "requestId";
constexpr char kReasonKey[] = "reason";

constexpr char kCancelledReason[] = "The client cancelled the request.";
constexpr char kTimedOutReason[] = "The request timed out.";
constexpr char kShutdownReason[] = "The client shut down.";
constexpr char kMethodNotFoundMessage[] = "Method not found";

// Bound how much server-supplied text is echoed in an error.
constexpr size_t kMaxReportedVersionLength = 32;
constexpr size_t kMaxReportedServerMessageLength = 500;

// Each server request is answered with a POST, so bound how many requests a
// server can make the client send from one response.
constexpr size_t kMaxServerRequestsPerRequest = 16;

// The MCP spec limits session ids to visible ASCII.
bool IsValidSessionId(std::string_view session_id) {
  return !session_id.empty() && std::ranges::all_of(session_id, [](char c) {
    return c > 0x20 && c < 0x7f;
  });
}

// Truncates server-supplied `text` to at most `max_bytes` and replaces
// control, format and line-breaking characters, such as bidi overrides, which
// could otherwise restructure or disguise the text it's shown in.
std::string SanitizeServerText(std::string_view text, size_t max_bytes) {
  std::string truncated;
  base::TruncateUTF8ToByteSize(text, max_bytes, &truncated);
  std::string sanitized;
  sanitized.reserve(truncated.size());
  for (size_t i = 0; i < truncated.size(); ++i) {
    base_icu::UChar32 code_point;
    if (!base::ReadUnicodeCharacter(truncated, &i, &code_point)) {
      sanitized.push_back('?');
      continue;
    }
    switch (u_charType(code_point)) {
      case U_CONTROL_CHAR:
      case U_FORMAT_CHAR:
      case U_LINE_SEPARATOR:
      case U_PARAGRAPH_SEPARATOR:
        sanitized.push_back('?');
        break;
      default:
        base::WriteUnicodeCharacter(code_point, &sanitized);
    }
  }
  return sanitized;
}

std::vector<MCPToolDefinition> CloneTools(
    const std::vector<MCPToolDefinition>& tools) {
  return base::ToVector(tools, &MCPToolDefinition::Clone);
}

bool IsSuccess(const MCPTransport::Response& response) {
  return network::IsSuccessfulStatus(response.status_code);
}

// The error for `code`, with a message naming server `name`.
MCPError MakeErrorFor(std::string_view name, MCPErrorCode code) {
  std::string message;
  switch (code) {
    case MCPErrorCode::kInvalidEndpoint:
      message = base::StrCat({"The URL of ", name,
                              " isn't allowed. Use an https:// URL, or "
                              "http:// for a server on localhost."});
      break;
    case MCPErrorCode::kInvalidToken:
      message = base::StrCat({"The access token for ", name,
                              " contains characters that can't be sent."});
      break;
    case MCPErrorCode::kInvalidRequest:
      message = base::StrCat({"The request to ", name, " couldn't be sent."});
      break;
    case MCPErrorCode::kNetworkError:
      message = base::StrCat({"Couldn't connect to ", name, "."});
      break;
    case MCPErrorCode::kTimeout:
      message = base::StrCat({name, " didn't respond in time."});
      break;
    case MCPErrorCode::kResponseTooLarge:
      message = base::StrCat(
          {name, " sent a response larger than the ",
           base::NumberToString(kMCPMaxResponseBytes / (1024 * 1024)),
           " MB limit."});
      break;
    case MCPErrorCode::kCrossOriginRedirect:
      message = base::StrCat(
          {name, " redirected to another site, which isn't followed."});
      break;
    case MCPErrorCode::kStreamableHttpNotSupported:
      message = base::StrCat(
          {name,
           " didn't accept an MCP connection. If it only supports the older "
           "HTTP+SSE transport, use its Streamable HTTP URL instead, which "
           "usually ends in /mcp."});
      break;
    case MCPErrorCode::kAuthRequired:
      message = base::StrCat(
          {name, " requires an access token. Add one to its settings."});
      break;
    case MCPErrorCode::kAuthRejected:
      message = base::StrCat(
          {name,
           " rejected the access token. Check that it's correct and "
           "hasn't expired."});
      break;
    case MCPErrorCode::kUnsupportedProtocolVersion:
      message = base::StrCat(
          {name, " uses an MCP protocol version that isn't supported."});
      break;
    case MCPErrorCode::kToolsNotSupported:
      message = base::StrCat({name, " doesn't provide any tools."});
      break;
    case MCPErrorCode::kSessionExpired:
      message = base::StrCat({name, " ended the session."});
      break;
    case MCPErrorCode::kNoResponse:
      message =
          base::StrCat({name, " closed the connection without responding."});
      break;
    case MCPErrorCode::kInvalidResponse:
      message =
          base::StrCat({name,
                        " sent an invalid response. Check that its URL is the "
                        "server's MCP endpoint."});
      break;
    case MCPErrorCode::kServerError:
      message = base::StrCat({name, " returned an error."});
      break;
    case MCPErrorCode::kHttpError:
      message = base::StrCat({name, " returned an HTTP error."});
      break;
  }
  return MCPError{code, std::move(message)};
}

}  // namespace

struct MCPClient::Request {
  std::string method;
  // Kept so the request can be resent after the session expires.
  std::optional<base::DictValue> params;
  ResultCallback callback;
  // Set while the request is on the wire.
  std::optional<MCPTransport::RequestId> transport_request;
  // The session headers it was sent with, which replies and cancellations
  // concerning it must also carry.
  MCPTransport::Headers sent_headers;
  int session_generation = 0;
  bool retried = false;
  // A response-shaped message on its stream was malformed or unmatched.
  bool received_invalid_response = false;
  size_t server_requests_answered = 0;
  base::TimeTicks dispatch_time;
  base::OneShotTimer timeout_timer;
};

// static
base::expected<std::unique_ptr<MCPClient>, MCPError> MCPClient::Create(
    MCPServerConfig config,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  if (ValidateMCPEndpoint(config.endpoint) !=
      MCPEndpointValidationResult::kValid) {
    return base::unexpected(
        MakeErrorFor(config.display_name, MCPErrorCode::kInvalidEndpoint));
  }
  if (config.bearer_token &&
      !net::HttpUtil::IsValidHeaderValue(*config.bearer_token)) {
    return base::unexpected(
        MakeErrorFor(config.display_name, MCPErrorCode::kInvalidToken));
  }
  const bool has_bearer_token = config.bearer_token.has_value();
  return std::make_unique<MCPClient>(
      std::move(config.display_name), has_bearer_token,
      std::make_unique<MCPStreamableHttpTransport>(
          std::move(config.endpoint), std::move(config.bearer_token),
          kMCPMaxResponseBytes, std::move(url_loader_factory)));
}

MCPClient::MCPClient(std::string display_name,
                     bool has_bearer_token,
                     std::unique_ptr<MCPTransport> transport)
    : display_name_(std::move(display_name)),
      has_bearer_token_(has_bearer_token),
      transport_(std::move(transport)) {
  CHECK(transport_);
}

MCPClient::~MCPClient() {
  // A disconnect doesn't stop the server's work, so it must be told.
  for (const auto& [id, request] : requests_) {
    if (request->transport_request && request->method != kInitializeMethod) {
      SendCancelledNotification(id, request->sent_headers, kShutdownReason);
    }
  }
  if (session_id_) {
    transport_->TerminateSession(SessionHeaders());
  }
}

void MCPClient::GetTools(GetToolsCallback callback) {
  if (tools_cache_ &&
      base::TimeTicks::Now() - tools_cache_time_ < kMCPToolsCacheTtl) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&MCPClient::RunGetToolsCallback,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       CloneTools(*tools_cache_)));
    return;
  }
  get_tools_callbacks_.push_back(std::move(callback));
  if (get_tools_callbacks_.size() == 1) {
    tools_in_progress_.clear();
    tools_pages_fetched_ = 0;
    RequestToolsPage(std::nullopt);
  }
}

base::OnceClosure MCPClient::CallTool(std::string_view name,
                                      base::DictValue arguments,
                                      CallToolCallback callback) {
  const int id = ++next_request_id_;
  base::DictValue params;
  params.Set(kNameKey, name);
  params.Set(kArgumentsKey, std::move(arguments));
  // Lets the server report progress, which extends the timeout.
  params.Set(kMetaKey, base::DictValue().Set(kProgressTokenKey, id));
  SendRequest(
      id, kToolsCallMethod, std::move(params),
      base::BindOnce(&MCPClient::OnCallToolResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  return base::BindOnce(&MCPClient::CancelRequest,
                        weak_ptr_factory_.GetWeakPtr(), id);
}

base::WeakPtr<MCPClient> MCPClient::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void MCPClient::SendRequest(int id,
                            std::string_view method,
                            std::optional<base::DictValue> params,
                            ResultCallback callback) {
  auto request = std::make_unique<Request>();
  request->method = std::string(method);
  request->params = std::move(params);
  request->callback = std::move(callback);
  requests_.emplace(id, std::move(request));
  if (method == kInitializeMethod) {
    Dispatch(id);
  } else {
    ScheduleRequest(id);
  }
}

void MCPClient::ScheduleRequest(int id) {
  if (state_ == State::kReady) {
    Dispatch(id);
    return;
  }
  queued_requests_.push_back(id);
  if (state_ != State::kInitializing) {
    StartInitialize();
  }
}

void MCPClient::Dispatch(int id) {
  Request& request = *requests_.at(id);
  const bool is_initialize = request.method == kInitializeMethod;
  // Nothing about the session applies until `initialize` has been answered.
  request.sent_headers =
      is_initialize ? MCPTransport::Headers() : SessionHeaders();
  request.session_generation = session_generation_;
  request.dispatch_time = base::TimeTicks::Now();
  std::optional<base::DictValue> params;
  if (request.params) {
    params = request.params->Clone();
  }
  request.transport_request = transport_->Send(
      MakeJsonRpcRequest(id, request.method, std::move(params)),
      request.sent_headers, kMCPMaxRequestTimeout,
      base::BindRepeating(&MCPClient::OnMessage, weak_ptr_factory_.GetWeakPtr(),
                          id),
      base::BindOnce(&MCPClient::OnRequestComplete,
                     weak_ptr_factory_.GetWeakPtr(), id));
  StartTimeout(request, id);
}

void MCPClient::StartTimeout(Request& request, int id) {
  const base::TimeDelta until_max =
      request.dispatch_time + kMCPMaxRequestTimeout - base::TimeTicks::Now();
  request.timeout_timer.Start(
      FROM_HERE, std::min(kMCPRequestTimeout, until_max),
      base::BindOnce(&MCPClient::OnRequestTimeout, base::Unretained(this), id));
}

void MCPClient::OnMessage(int sent_request_id,
                          base::DictValue message,
                          const MCPTransport::Headers& response_headers) {
  const bool is_response_shaped = IsJsonRpcResponseShaped(message);
  std::optional<JsonRpcMessage> parsed =
      ParseJsonRpcMessage(std::move(message));
  if (!parsed) {
    DVLOG(1) << "Ignoring a malformed MCP message";
    if (is_response_shaped) {
      MarkInvalidResponse(sent_request_id);
    }
    return;
  }

  if (auto* server_request = std::get_if<JsonRpcRequest>(&*parsed)) {
    auto it = requests_.find(sent_request_id);
    if (it == requests_.end() ||
        it->second->server_requests_answered >= kMaxServerRequestsPerRequest) {
      return;
    }
    ++it->second->server_requests_answered;
    MCPTransport::Headers reply_headers = it->second->sent_headers;
    // An `initialize` is sent before there's a session, which is identified
    // only by its own response.
    if (it->second->method == kInitializeMethod) {
      if (auto session = response_headers.find(kSessionIdResponseHeader);
          session != response_headers.end() &&
          IsValidSessionId(session->second)) {
        reply_headers[kSessionIdHeader] = session->second;
      }
    }
    ReplyToServerRequest(*server_request, reply_headers);
    return;
  }

  if (auto* notification = std::get_if<JsonRpcNotification>(&*parsed)) {
    if (notification->method != kProgressNotification ||
        !notification->params) {
      return;
    }
    std::optional<int> token = notification->params->FindInt(kProgressTokenKey);
    if (!token) {
      return;
    }
    auto it = requests_.find(*token);
    if (it != requests_.end() && it->second->transport_request) {
      StartTimeout(*it->second, *token);
    }
    return;
  }

  auto& response = std::get<JsonRpcResponse>(*parsed);
  // An error without an id refers to the request whose reply carried it.
  int id = sent_request_id;
  if (response.id) {
    // Ids sent are integers, so a string id answers nothing.
    const int* response_id = std::get_if<int>(&*response.id);
    if (!response_id) {
      MarkInvalidResponse(sent_request_id);
      return;
    }
    id = *response_id;
  }
  auto it = requests_.find(id);
  if (it == requests_.end() || !it->second->transport_request) {
    MarkInvalidResponse(sent_request_id);
    return;
  }
  if (!response.result.has_value()) {
    CompleteRequest(id,
                    base::unexpected(MakeServerError(response.result.error())));
    return;
  }
  // Completing drops the connection, so a server that leaves the stream open
  // or fails after responding doesn't affect the result.
  CompleteRequest(id, std::move(*response.result), response_headers);
}

void MCPClient::MarkInvalidResponse(int id) {
  auto it = requests_.find(id);
  if (it != requests_.end()) {
    it->second->received_invalid_response = true;
  }
}

void MCPClient::OnRequestComplete(int id, TransportResult result) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  Request& request = *it->second;
  request.transport_request.reset();
  request.timeout_timer.Stop();

  if (!result.has_value()) {
    if (result.error().code == MCPErrorCode::kTimeout) {
      OnRequestTimeout(id);
      return;
    }
    CompleteRequest(id, base::unexpected(MakeTransportError(result.error())));
    return;
  }

  const int status = result->status_code;
  if (IsSuccess(*result)) {
    // Not retried: a request such as `tools/call` may already have run.
    CompleteRequest(
        id, base::unexpected(MakeError(request.received_invalid_response
                                           ? MCPErrorCode::kInvalidResponse
                                           : MCPErrorCode::kNoResponse)));
    return;
  }

  // The session expired, and the server didn't process the request.
  if (status == net::HTTP_NOT_FOUND &&
      request.sent_headers.contains(kSessionIdHeader)) {
    if (request.retried) {
      CompleteRequest(
          id, base::unexpected(MakeError(MCPErrorCode::kSessionExpired)));
      return;
    }
    request.retried = true;
    // Other requests on the same session may already have re-initialized.
    if (request.session_generation == session_generation_) {
      ResetSession();
    }
    ScheduleRequest(id);
    return;
  }

  CompleteRequest(id, base::unexpected(MakeHttpError(
                          *result, request.method == kInitializeMethod)));
}

void MCPClient::OnRequestTimeout(int id) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  // A disconnect doesn't stop the server's work, so it must be told. The spec
  // forbids cancelling `initialize`.
  if (it->second->method != kInitializeMethod) {
    SendCancelledNotification(id, it->second->sent_headers, kTimedOutReason);
  }
  CompleteRequest(id, base::unexpected(MakeError(MCPErrorCode::kTimeout)));
}

void MCPClient::CompleteRequest(
    int id,
    base::expected<base::DictValue, MCPError> result,
    const MCPTransport::Headers& response_headers) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  std::unique_ptr<Request> request = std::move(it->second);
  requests_.erase(it);
  std::erase(queued_requests_, id);
  if (request->transport_request) {
    transport_->Cancel(*request->transport_request);
  }
  // May destroy `this`.
  std::move(request->callback).Run(std::move(result), response_headers);
}

void MCPClient::CancelRequest(int id) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  std::unique_ptr<Request> request = std::move(it->second);
  requests_.erase(it);
  std::erase(queued_requests_, id);
  if (request->transport_request) {
    transport_->Cancel(*request->transport_request);
    // Closing the connection doesn't stop the server's work.
    SendCancelledNotification(id, request->sent_headers, kCancelledReason);
  }
}

void MCPClient::SendCancelledNotification(int id,
                                          const MCPTransport::Headers& headers,
                                          std::string_view reason) {
  // Detached, so that it isn't lost if the client is destroyed next.
  transport_->SendDetached(
      MakeJsonRpcNotification(
          kCancelledNotification,
          base::DictValue().Set(kRequestIdKey, id).Set(kReasonKey, reason)),
      headers);
}

void MCPClient::SendNotification(std::string_view method,
                                 std::optional<base::DictValue> params,
                                 MCPTransport::CompleteCallback on_complete) {
  transport_->Send(MakeJsonRpcNotification(method, std::move(params)),
                   SessionHeaders(), kMCPRequestTimeout, base::DoNothing(),
                   std::move(on_complete));
}

void MCPClient::ReplyToServerRequest(const JsonRpcRequest& request,
                                     const MCPTransport::Headers& headers) {
  // The client declares no capabilities, so a ping is the only request a
  // server may send.
  base::DictValue reply =
      request.method == kPingMethod
          ? MakeJsonRpcResult(request.id, base::DictValue())
          : MakeJsonRpcErrorResponse(request.id, kJsonRpcMethodNotFound,
                                     kMethodNotFoundMessage);
  transport_->Send(reply, headers, kMCPRequestTimeout, base::DoNothing(),
                   base::DoNothing());
}

MCPTransport::Headers MCPClient::SessionHeaders() const {
  MCPTransport::Headers headers;
  if (protocol_version_) {
    headers[kProtocolVersionHeader] = *protocol_version_;
  }
  if (session_id_) {
    headers[kSessionIdHeader] = *session_id_;
  }
  return headers;
}

void MCPClient::ResetSession() {
  protocol_version_.reset();
  session_id_.reset();
  ++session_generation_;
  state_ = State::kIdle;
}

void MCPClient::StartInitialize() {
  state_ = State::kInitializing;
  base::DictValue params;
  params.Set(kProtocolVersionKey, kLatestProtocolVersion);
  // No client features, so servers must not send sampling, roots or
  // elicitation requests.
  params.Set(kCapabilitiesKey, base::DictValue());
  params.Set(
      kClientInfoKey,
      base::DictValue()
          .Set(kNameKey, kClientName)
          .Set(kVersionKey,
               version_info::GetBraveVersionWithoutChromiumMajorVersion()));
  SendRequest(++next_request_id_, kInitializeMethod, std::move(params),
              base::BindOnce(&MCPClient::OnInitializeResponse,
                             weak_ptr_factory_.GetWeakPtr()));
}

void MCPClient::OnInitializeResponse(
    base::expected<base::DictValue, MCPError> result,
    const MCPTransport::Headers& response_headers) {
  if (!result.has_value()) {
    FailInitialize(std::move(result.error()));
    return;
  }

  // Captured first so that a failed handshake can still end the session.
  if (auto it = response_headers.find(kSessionIdResponseHeader);
      it != response_headers.end()) {
    if (!IsValidSessionId(it->second)) {
      FailInitialize(MakeError(MCPErrorCode::kInvalidResponse));
      return;
    }
    session_id_ = it->second;
  }

  const std::string* version = result->FindString(kProtocolVersionKey);
  if (!version) {
    FailInitialize(MakeError(MCPErrorCode::kInvalidResponse));
    return;
  }
  if (!kSupportedProtocolVersions.contains(*version)) {
    MCPError error = MakeError(MCPErrorCode::kUnsupportedProtocolVersion);
    error.message = base::StrCat(
        {display_name_, " uses MCP protocol version \"",
         SanitizeServerText(*version, kMaxReportedVersionLength),
         "\", which isn't supported. Supported versions: ",
         base::JoinString(base::ToVector(kSupportedProtocolVersions), ", "),
         "."});
    FailInitialize(std::move(error));
    return;
  }
  protocol_version_ = *version;

  const base::DictValue* capabilities = result->FindDict(kCapabilitiesKey);
  if (!capabilities || !capabilities->FindDict(kToolsKey)) {
    FailInitialize(MakeError(MCPErrorCode::kToolsNotSupported));
    return;
  }

  SendNotification(kInitializedNotification, std::nullopt,
                   base::BindOnce(&MCPClient::OnInitializedNotificationComplete,
                                  weak_ptr_factory_.GetWeakPtr()));
}

void MCPClient::OnInitializedNotificationComplete(TransportResult result) {
  if (!result.has_value()) {
    FailInitialize(MakeTransportError(result.error()));
    return;
  }
  // Expected to be `202 Accepted`.
  if (!IsSuccess(*result)) {
    FailInitialize(MakeHttpError(*result, /*is_initialize=*/false));
    return;
  }
  state_ = State::kReady;
  last_error_.reset();
  for (int id : std::exchange(queued_requests_, {})) {
    Dispatch(id);
  }
}

void MCPClient::FailInitialize(MCPError error) {
  if (session_id_) {
    transport_->TerminateSession(SessionHeaders());
  }
  ResetSession();
  state_ = State::kFailed;
  last_error_ = error;
  tools_cache_.reset();

  base::WeakPtr<MCPClient> weak_this = weak_ptr_factory_.GetWeakPtr();
  for (int id : std::exchange(queued_requests_, {})) {
    CompleteRequest(id, base::unexpected(error));
    if (!weak_this) {
      return;
    }
  }
}

void MCPClient::RequestToolsPage(std::optional<std::string> cursor) {
  std::optional<base::DictValue> params;
  if (cursor) {
    params = base::DictValue().Set(kCursorKey, std::move(*cursor));
  }
  SendRequest(++next_request_id_, kToolsListMethod, std::move(params),
              base::BindOnce(&MCPClient::OnToolsListPage,
                             weak_ptr_factory_.GetWeakPtr()));
}

void MCPClient::OnToolsListPage(
    base::expected<base::DictValue, MCPError> result,
    const MCPTransport::Headers& response_headers) {
  if (!result.has_value()) {
    FinishGetTools(base::unexpected(std::move(result.error())));
    return;
  }
  const base::ListValue* tools = result->FindList(kToolsKey);
  if (!tools) {
    FinishGetTools(base::unexpected(MakeError(MCPErrorCode::kInvalidResponse)));
    return;
  }
  for (const base::Value& tool : *tools) {
    if (tools_in_progress_.size() >= kMCPMaxToolsPerServer) {
      break;
    }
    std::optional<MCPToolDefinition> definition =
        tool.is_dict() ? ParseToolDefinition(tool.GetDict()) : std::nullopt;
    // Names identify tools, so a duplicate would be ambiguous.
    if (!definition ||
        std::ranges::contains(tools_in_progress_, definition->name,
                              &MCPToolDefinition::name)) {
      DVLOG(1) << "Ignoring a malformed or duplicate MCP tool";
      continue;
    }
    tools_in_progress_.push_back(std::move(*definition));
  }
  ++tools_pages_fetched_;

  const std::string* next_cursor = result->FindString(kNextCursorKey);
  if (next_cursor && !next_cursor->empty() &&
      tools_pages_fetched_ < kMCPMaxToolsListPages &&
      tools_in_progress_.size() < kMCPMaxToolsPerServer) {
    RequestToolsPage(*next_cursor);
    return;
  }
  FinishGetTools(std::exchange(tools_in_progress_, {}));
}

void MCPClient::FinishGetTools(
    base::expected<std::vector<MCPToolDefinition>, MCPError> result) {
  tools_in_progress_.clear();
  if (result.has_value()) {
    tools_cache_ = CloneTools(*result);
    tools_cache_time_ = base::TimeTicks::Now();
  }
  base::WeakPtr<MCPClient> weak_this = weak_ptr_factory_.GetWeakPtr();
  for (GetToolsCallback& callback : std::exchange(get_tools_callbacks_, {})) {
    if (result.has_value()) {
      std::move(callback).Run(CloneTools(*result));
    } else {
      std::move(callback).Run(base::unexpected(result.error()));
    }
    if (!weak_this) {
      return;
    }
  }
}

void MCPClient::RunGetToolsCallback(GetToolsCallback callback,
                                    std::vector<MCPToolDefinition> tools) {
  std::move(callback).Run(std::move(tools));
}

void MCPClient::OnCallToolResponse(
    CallToolCallback callback,
    base::expected<base::DictValue, MCPError> result,
    const MCPTransport::Headers& response_headers) {
  if (!result.has_value()) {
    std::move(callback).Run(base::unexpected(std::move(result.error())));
    return;
  }
  std::optional<MCPCallToolResult> call_result =
      ParseCallToolResult(std::move(*result));
  if (!call_result) {
    std::move(callback).Run(
        base::unexpected(MakeError(MCPErrorCode::kInvalidResponse)));
    return;
  }
  std::move(callback).Run(std::move(*call_result));
}

MCPError MCPClient::MakeError(MCPErrorCode code) const {
  return MakeErrorFor(display_name_, code);
}

MCPError MCPClient::MakeTransportError(const MCPTransport::Error& error) const {
  MCPError result = MakeError(error.code);
  if (error.code == MCPErrorCode::kNetworkError) {
    result.message =
        base::StrCat({"Couldn't connect to ", display_name_, " (",
                      net::ErrorToShortString(error.net_error), ")."});
  } else if (error.code == MCPErrorCode::kCrossOriginRedirect) {
    result.message = base::StrCat(
        {display_name_, " redirected to ",
         url::Origin::Create(error.redirect_url).Serialize(),
         ", a different site, so the redirect wasn't followed. If you trust "
         "that site, use the server's URL there instead."});
  }
  return result;
}

MCPError MCPClient::MakeHttpError(const MCPTransport::Response& response,
                                  bool is_initialize) const {
  const int status = response.status_code;
  if (status == net::HTTP_UNAUTHORIZED || status == net::HTTP_FORBIDDEN) {
    return MakeError(has_bearer_token_ ? MCPErrorCode::kAuthRejected
                                       : MCPErrorCode::kAuthRequired);
  }
  if (response.error_body) {
    std::optional<JsonRpcMessage> message =
        ParseJsonRpcMessage(response.error_body->Clone());
    if (auto* error_response =
            message ? std::get_if<JsonRpcResponse>(&*message) : nullptr;
        error_response && !error_response->result.has_value()) {
      return MakeServerError(error_response->result.error());
    }
  }
  // What a server that only speaks the deprecated HTTP+SSE transport answers.
  if (is_initialize &&
      (status == net::HTTP_BAD_REQUEST || status == net::HTTP_NOT_FOUND ||
       status == net::HTTP_METHOD_NOT_ALLOWED)) {
    return MakeError(MCPErrorCode::kStreamableHttpNotSupported);
  }
  MCPError error = MakeError(MCPErrorCode::kHttpError);
  error.message = base::StrCat(
      {display_name_, " returned HTTP ", base::NumberToString(status), "."});
  return error;
}

MCPError MCPClient::MakeServerError(const JsonRpcError& error) const {
  return MCPError{
      MCPErrorCode::kServerError,
      base::StrCat({display_name_, " returned an error: ",
                    SanitizeServerText(error.message,
                                       kMaxReportedServerMessageLength)})};
}

}  // namespace ai_chat
