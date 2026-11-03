/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_streamable_http_transport.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_http_test_base.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::test::ParseJsonDict;

namespace ai_chat {

namespace {

constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
constexpr char kToken[] = "secret-token";
constexpr size_t kMaxResponseBytes = 1024;
constexpr base::TimeDelta kTimeout = base::Seconds(30);

constexpr char kRequest[] = R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})";
constexpr char kResponse[] =
    R"({"jsonrpc":"2.0","id":1,"result":{"tools":[]}})";

using TransportResult =
    base::expected<MCPTransport::Response, MCPTransport::Error>;

struct Call {
  std::vector<base::DictValue> messages;
  // The response headers that came with each message.
  std::vector<MCPTransport::Headers> message_headers;
  base::test::TestFuture<TransportResult> result;
};

}  // namespace

class MCPStreamableHttpTransportTest : public MCPHttpTestBase {
 protected:
  MCPStreamableHttpTransportTest()
      : transport_(std::make_unique<MCPStreamableHttpTransport>(
            GURL(kEndpoint),
            kToken,
            kMaxResponseBytes,
            url_loader_factory_.GetSafeWeakWrapper())) {}

  void TearDown() override {
    transport_.reset();
    MCPHttpTestBase::TearDown();
  }

  // Sends `kRequest` and returns the index of its pending request.
  size_t Send(Call& call,
              const MCPTransport::Headers& headers = {},
              base::TimeDelta timeout = kTimeout) {
    transport_->Send(
        ParseJsonDict(kRequest), headers, timeout,
        base::BindLambdaForTesting(
            [&call](base::DictValue m, const MCPTransport::Headers& headers) {
              call.messages.push_back(std::move(m));
              call.message_headers.push_back(headers);
            }),
        call.result.GetCallback());
    EXPECT_FALSE(url_loader_factory_.pending_requests()->empty());
    return url_loader_factory_.pending_requests()->size() - 1;
  }

  std::unique_ptr<MCPStreamableHttpTransport> transport_;
};

TEST_F(MCPStreamableHttpTransportTest, PostsMessageWithHeaders) {
  Call call;
  const size_t index = Send(call, {{"MCP-Protocol-Version", "2025-06-18"},
                                   {"Mcp-Session-Id", "session-1"}});
  const network::ResourceRequest& request = Pending(index).request;

  EXPECT_EQ(request.url, GURL(kEndpoint));
  EXPECT_EQ(request.method, "POST");
  EXPECT_EQ(base::test::ParseJson(network::GetUploadData(request)),
            base::test::ParseJson(kRequest));
  EXPECT_EQ(request.headers.GetHeader("Accept"),
            "application/json, text/event-stream");
  EXPECT_EQ(request.headers.GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(request.headers.GetHeader("Authorization"), "Bearer secret-token");
  EXPECT_EQ(request.headers.GetHeader("MCP-Protocol-Version"), "2025-06-18");
  EXPECT_EQ(request.headers.GetHeader("Mcp-Session-Id"), "session-1");

  // No cookies or other ambient credentials.
  EXPECT_EQ(request.credentials_mode, network::mojom::CredentialsMode::kOmit);
  EXPECT_TRUE(request.load_flags & net::LOAD_DO_NOT_SAVE_COOKIES);
  EXPECT_TRUE(request.load_flags & net::LOAD_DISABLE_CACHE);
  EXPECT_FALSE(request.headers.HasHeader("Cookie"));
}

TEST_F(MCPStreamableHttpTransportTest, NoAuthorizationWithoutToken) {
  transport_ = std::make_unique<MCPStreamableHttpTransport>(
      GURL(kEndpoint), std::nullopt, kMaxResponseBytes,
      url_loader_factory_.GetSafeWeakWrapper());
  Call call;
  const size_t index = Send(call);
  EXPECT_FALSE(Pending(index).request.headers.HasHeader("Authorization"));
}

TEST_F(MCPStreamableHttpTransportTest, JsonResponse) {
  Call call;
  Respond(Send(call), net::HTTP_OK, "application/json", kResponse,
          {{"Mcp-Session-Id", "session-1"}});

  TransportResult result = Take(call.result);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->status_code, net::HTTP_OK);
  EXPECT_EQ(result->headers.at("mcp-session-id"), "session-1");
  EXPECT_FALSE(result->error_body);
  ASSERT_EQ(call.messages.size(), 1u);
  EXPECT_EQ(call.messages[0], ParseJsonDict(kResponse));
  EXPECT_EQ(call.message_headers[0].at("mcp-session-id"), "session-1");
}

TEST_F(MCPStreamableHttpTransportTest, SseResponseWithNotificationsFirst) {
  Call call;
  Respond(Send(call), net::HTTP_OK, "text/event-stream",
          base::StrCat({
              // The priming event, which carries no data.
              "id: 1\ndata:\n\n",
              ": keepalive\n\n",
              "event: message\ndata: {\"jsonrpc\":\"2.0\",\"method\":"
              "\"notifications/progress\",\"params\":{\"progressToken\":1,"
              "\"progress\":1}}\n\n",
              "data: {\"jsonrpc\":\"2.0\",\"id\":\"s1\",\"method\":\"ping\"}"
              "\n\n",
              "data: {not json\n\n",
              "data:",
              kResponse,
              "\n\n",
          }),
          {{"Mcp-Session-Id", "session-1"}});

  TransportResult result = Take(call.result);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->status_code, net::HTTP_OK);
  EXPECT_EQ(result->headers.at("mcp-session-id"), "session-1");
  ASSERT_EQ(call.messages.size(), 3u);
  for (const MCPTransport::Headers& headers : call.message_headers) {
    // Available with each message, before the stream ends.
    EXPECT_EQ(headers.at("mcp-session-id"), "session-1");
  }
  EXPECT_EQ(*call.messages[0].FindString("method"), "notifications/progress");
  EXPECT_EQ(*call.messages[1].FindString("method"), "ping");
  EXPECT_EQ(call.messages[2], ParseJsonDict(kResponse));
}

TEST_F(MCPStreamableHttpTransportTest, AcceptedWithoutBody) {
  for (int status : {net::HTTP_ACCEPTED, net::HTTP_OK}) {
    SCOPED_TRACE(status);
    Call call;
    Respond(Send(call), status, "", "");
    TransportResult result = Take(call.result);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status_code, status);
    EXPECT_TRUE(call.messages.empty());
  }
}

TEST_F(MCPStreamableHttpTransportTest, ErrorStatusWithJsonRpcBody) {
  constexpr char kErrorBody[] =
      R"({"jsonrpc":"2.0","id":null,
          "error":{"code":-32600,"message":"Bad session"}})";
  Call call;
  Respond(Send(call), net::HTTP_BAD_REQUEST, "application/json", kErrorBody);

  TransportResult result = Take(call.result);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->status_code, net::HTTP_BAD_REQUEST);
  EXPECT_EQ(result->error_body, ParseJsonDict(kErrorBody));
  EXPECT_TRUE(call.messages.empty());
}

TEST_F(MCPStreamableHttpTransportTest, ErrorStatusWithoutJsonBody) {
  for (int status :
       {net::HTTP_BAD_REQUEST, net::HTTP_NOT_FOUND,
        net::HTTP_METHOD_NOT_ALLOWED, net::HTTP_INTERNAL_SERVER_ERROR}) {
    SCOPED_TRACE(status);
    Call call;
    Respond(Send(call), status, "text/html", "<html>Not here</html>");
    TransportResult result = Take(call.result);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status_code, status);
    EXPECT_FALSE(result->error_body);
    EXPECT_TRUE(call.messages.empty());
  }
}

TEST_F(MCPStreamableHttpTransportTest, InvalidSuccessBody) {
  for (const auto [mime_type, body] :
       {std::pair("text/html", "<html></html>"),
        std::pair("application/json", "{not json"),
        std::pair("application/json", "[]"), std::pair("", "hello")}) {
    SCOPED_TRACE(body);
    Call call;
    Respond(Send(call), net::HTTP_OK, mime_type, body);
    TransportResult result = Take(call.result);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, MCPErrorCode::kInvalidResponse);
    EXPECT_TRUE(call.messages.empty());
  }
}

TEST_F(MCPStreamableHttpTransportTest, ResponseOverByteCap) {
  const std::string padding(kMaxResponseBytes, ' ');
  const std::string json_body = base::StrCat({kResponse, padding});
  const std::string sse_body =
      base::StrCat({"data: ", kResponse, "\n\n", ": ", padding, "\n\n"});
  for (const auto [mime_type, body] :
       {std::pair("application/json", std::string_view(json_body)),
        std::pair("text/event-stream", std::string_view(sse_body))}) {
    SCOPED_TRACE(mime_type);
    Call call;
    Respond(Send(call), net::HTTP_OK, mime_type, body);
    TransportResult result = Take(call.result);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, MCPErrorCode::kResponseTooLarge);
  }
}

TEST_F(MCPStreamableHttpTransportTest, Timeout) {
  Call call;
  Send(call);
  task_environment_.FastForwardBy(kTimeout - base::Milliseconds(1));
  EXPECT_FALSE(call.result.IsReady());
  task_environment_.FastForwardBy(base::Milliseconds(1));
  ASSERT_TRUE(call.result.IsReady());
  TransportResult result = Take(call.result);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kTimeout);
}

TEST_F(MCPStreamableHttpTransportTest, NetworkError) {
  Call call;
  url_loader_factory_.SimulateResponseWithoutRemovingFromPendingList(
      &Pending(Send(call)), MakeMCPResponseHead(net::HTTP_OK, ""), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));
  TransportResult result = Take(call.result);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), (MCPTransport::Error{MCPErrorCode::kNetworkError,
                                                 net::ERR_CONNECTION_REFUSED}));
}

TEST_F(MCPStreamableHttpTransportTest, CancelDropsCallbacks) {
  const MCPTransport::RequestId id = transport_->Send(
      ParseJsonDict(kRequest), {}, kTimeout,
      base::BindLambdaForTesting(
          [](base::DictValue, const MCPTransport::Headers&) { ADD_FAILURE(); }),
      base::BindOnce([](TransportResult) { ADD_FAILURE(); }));
  const size_t index = url_loader_factory_.pending_requests()->size() - 1;
  transport_->Cancel(id);
  RunReadyTasks();
  EXPECT_FALSE(Pending(index).client.is_connected());
  task_environment_.FastForwardBy(kTimeout);
}

TEST_F(MCPStreamableHttpTransportTest, DestroyFromMessageCallback) {
  const size_t index_before = url_loader_factory_.pending_requests()->size();
  size_t message_count = 0;
  transport_->Send(ParseJsonDict(kRequest), {}, kTimeout,
                   base::BindLambdaForTesting(
                       [&](base::DictValue, const MCPTransport::Headers&) {
                         ++message_count;
                         transport_.reset();
                       }),
                   base::BindOnce([](TransportResult) { ADD_FAILURE(); }));
  Respond(index_before, net::HTTP_OK, "text/event-stream",
          base::StrCat({"data: ", kResponse, "\n\ndata: ", kResponse, "\n\n"}));
  RunReadyTasks();
  EXPECT_EQ(message_count, 1u);
  EXPECT_FALSE(transport_);
}

TEST_F(MCPStreamableHttpTransportTest, TerminateSession) {
  transport_->TerminateSession({{"Mcp-Session-Id", "session-1"},
                                {"MCP-Protocol-Version", "2025-11-25"}});
  ASSERT_EQ(url_loader_factory_.pending_requests()->size(), 1u);
  // The DELETE survives the transport.
  transport_.reset();

  const network::ResourceRequest& request = Pending(0).request;
  EXPECT_EQ(request.url, GURL(kEndpoint));
  EXPECT_EQ(request.method, "DELETE");
  EXPECT_EQ(request.headers.GetHeader("Mcp-Session-Id"), "session-1");
  EXPECT_EQ(request.headers.GetHeader("MCP-Protocol-Version"), "2025-11-25");
  EXPECT_EQ(request.headers.GetHeader("Authorization"), "Bearer secret-token");
  EXPECT_EQ(request.credentials_mode, network::mojom::CredentialsMode::kOmit);
  EXPECT_TRUE(request.load_flags & net::LOAD_DO_NOT_SAVE_COOKIES);
  ASSERT_TRUE(Pending(0).client.is_connected());

  // A server that doesn't let clients end sessions.
  Respond(0, net::HTTP_METHOD_NOT_ALLOWED, "text/plain", "Method Not Allowed");
  // Consumed without waiting for the timeout.
  RunReadyTasks();
  EXPECT_FALSE(Pending(0).client.is_connected());
}

TEST_F(MCPStreamableHttpTransportTest, SendDetached) {
  transport_->SendDetached(
      ParseJsonDict(R"({"jsonrpc":"2.0","method":"notifications/cancelled",
                        "params":{"requestId":1}})"),
      {{"Mcp-Session-Id", "session-1"}});
  ASSERT_EQ(url_loader_factory_.pending_requests()->size(), 1u);
  // The request survives the transport.
  transport_.reset();

  const network::ResourceRequest& request = Pending(0).request;
  EXPECT_EQ(request.method, "POST");
  EXPECT_EQ(base::test::ParseJson(network::GetUploadData(request)),
            base::test::ParseJson(
                R"({"jsonrpc":"2.0","method":"notifications/cancelled",
                    "params":{"requestId":1}})"));
  EXPECT_EQ(request.headers.GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(request.headers.GetHeader("Mcp-Session-Id"), "session-1");
  EXPECT_EQ(request.headers.GetHeader("Authorization"), "Bearer secret-token");
  EXPECT_EQ(request.credentials_mode, network::mojom::CredentialsMode::kOmit);
  Respond(0, net::HTTP_ACCEPTED, "", "");
  RunReadyTasks();
  EXPECT_FALSE(Pending(0).client.is_connected());
}

TEST_F(MCPStreamableHttpTransportTest, SameOriginRedirectIsFollowed) {
  Call call;
  const size_t index = Send(call);
  Redirect(index, GURL("https://mcp.example.com/mcp/"));
  RunReadyTasks();
  EXPECT_TRUE(Pending(index).client.is_connected());
  EXPECT_FALSE(call.result.IsReady());
}

TEST_F(MCPStreamableHttpTransportTest, CrossOriginRedirectIsRefused) {
  Call call;
  const size_t index = Send(call);
  Redirect(index, GURL("https://evil.example.com/steal?x=1"));
  TransportResult result = Take(call.result);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kCrossOriginRedirect);
  EXPECT_EQ(result.error().redirect_url,
            GURL("https://evil.example.com/steal?x=1"));
  EXPECT_FALSE(Pending(index).client.is_connected());
}

TEST_F(MCPStreamableHttpTransportTest, UnsafeSchemeRedirectIsNetworkError) {
  // Net refuses a redirect to a scheme it won't follow before the client sees
  // it, so the request still has the endpoint's URL.
  Call call;
  const size_t index = Send(call);
  Pending(index).client->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_UNSAFE_REDIRECT));
  TransportResult result = Take(call.result);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), (MCPTransport::Error{MCPErrorCode::kNetworkError,
                                                 net::ERR_UNSAFE_REDIRECT}));
}

TEST_F(MCPStreamableHttpTransportTest,
       DetachedRequestsRefuseCrossOriginRedirect) {
  transport_->SendDetached(ParseJsonDict(R"({"jsonrpc":"2.0","method":"n"})"),
                           {});
  transport_->TerminateSession({{"Mcp-Session-Id", "session-1"}});
  transport_.reset();
  ASSERT_EQ(url_loader_factory_.pending_requests()->size(), 2u);
  for (size_t index : {0u, 1u}) {
    SCOPED_TRACE(index);
    Redirect(index, GURL("https://evil.example.com/mcp"));
    RunReadyTasks();
    // Aborted rather than followed.
    EXPECT_FALSE(Pending(index).client.is_connected());
  }
}

}  // namespace ai_chat
