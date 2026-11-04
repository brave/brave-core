/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_http_test_base.h"
#include "brave/components/version_info/version_info.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
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
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::test::ParseJsonDict;
using testing::HasSubstr;

namespace ai_chat {

namespace {

constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
constexpr char kServerName[] = "Test Server";
constexpr char kSessionId[] = "session-1";

using Headers = MCPTestHeaders;
using ToolsResult = MCPClient::ToolsResult;
using CallResult = base::expected<MCPCallToolResult, MCPError>;

enum class Format { kJson, kSse };

std::string InitializeResult(std::string_view version, bool with_tools = true) {
  return base::StrCat({R"({"protocolVersion":")", version,
                       R"(","capabilities":)",
                       with_tools ? R"({"tools":{"listChanged":true}})" : "{}",
                       R"(,"serverInfo":{"name":"test","version":"1.0"}})"});
}

std::string ToolsListResult(
    const std::vector<std::string>& names,
    std::optional<std::string_view> next_cursor = std::nullopt) {
  base::ListValue tools;
  for (const std::string& name : names) {
    tools.Append(
        base::DictValue()
            .Set("name", name)
            .Set("inputSchema", base::DictValue().Set("type", "object")));
  }
  base::DictValue result;
  result.Set("tools", std::move(tools));
  if (next_cursor) {
    result.Set("nextCursor", *next_cursor);
  }
  return *base::WriteJson(result);
}

constexpr char kTextContentResult[] =
    R"({"content":[{"type":"text","text":"done"}]})";

std::vector<std::string> ToolNames(const ToolsResult& result) {
  std::vector<std::string> names;
  if (result.has_value()) {
    for (const MCPToolDefinition& tool : *result) {
      names.push_back(tool.name);
    }
  }
  return names;
}

}  // namespace

class MCPClientTest : public MCPHttpTestBase {
 protected:
  MCPClientTest() { CreateClient(); }

  void TearDown() override {
    client_.reset();
    MCPHttpTestBase::TearDown();
  }

  void CreateClient(std::optional<std::string> token = std::nullopt,
                    std::string_view endpoint = kEndpoint) {
    client_ = MCPClient::Create(MCPServerConfig(GURL(endpoint),
                                                std::move(token), kServerName),
                                url_loader_factory_.GetSafeWeakWrapper())
                  .value();
  }

  std::string Method(size_t index) {
    const base::DictValue body = Body(index);
    const std::string* method = body.FindString("method");
    return method ? *method : std::string();
  }

  int Id(size_t index) { return Body(index).FindInt("id").value_or(-1); }

  std::string ResultMessage(size_t index, std::string_view result) {
    return base::StrCat({R"({"jsonrpc":"2.0","id":)",
                         base::NumberToString(Id(index)), R"(,"result":)",
                         result, "}"});
  }

  void RespondResult(size_t index,
                     std::string_view result,
                     Format format = Format::kSse,
                     const Headers& headers = {}) {
    const std::string message = ResultMessage(index, result);
    if (format == Format::kSse) {
      Respond(index, net::HTTP_OK, "text/event-stream", MakeSSEEvent(message),
              headers);
    } else {
      Respond(index, net::HTTP_OK, "application/json", message, headers);
    }
  }

  // Answers the `initialize` at `index` and the `notifications/initialized`
  // that follows it. Returns the index of the next request.
  size_t CompleteHandshake(size_t index,
                           std::optional<std::string> session_id = kSessionId,
                           std::string_view version = "2025-11-25",
                           Format format = Format::kSse) {
    EXPECT_EQ(Method(index), "initialize");
    Headers headers;
    if (session_id) {
      headers["Mcp-Session-Id"] = *session_id;
    }
    RespondResult(index, InitializeResult(version), format, headers);
    EXPECT_EQ(Method(index + 1), "notifications/initialized");
    RespondAccepted(index + 1);
    return index + 2;
  }

  std::unique_ptr<MCPClient> client_;
};

TEST_F(MCPClientTest, InitializeHandshake) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  EXPECT_EQ(client_->state(), MCPClient::State::kInitializing);

  const base::DictValue initialize = Body(0);
  // Nothing else is sent until `initialize` is answered.
  EXPECT_EQ(RequestCount(), 1u);
  EXPECT_EQ(*initialize.FindString("jsonrpc"), "2.0");
  EXPECT_EQ(*initialize.FindString("method"), "initialize");
  EXPECT_TRUE(initialize.FindInt("id"));
  EXPECT_EQ(*initialize.FindDict("params"),
            ParseJsonDict(base::StrCat(
                {R"({"protocolVersion":"2025-11-25","capabilities":{},
               "clientInfo":{"name":"Brave","version":")",
                 version_info::GetBraveVersionWithoutChromiumMajorVersion(),
                 R"("}})"})));
  EXPECT_FALSE(Request(0).headers.HasHeader("MCP-Protocol-Version"));
  EXPECT_FALSE(Request(0).headers.HasHeader("Mcp-Session-Id"));
  EXPECT_FALSE(Request(0).headers.HasHeader("Authorization"));
  // No cookies or other ambient credentials.
  EXPECT_EQ(Request(0).credentials_mode,
            network::mojom::CredentialsMode::kOmit);
  EXPECT_TRUE(Request(0).load_flags & net::LOAD_DO_NOT_SAVE_COOKIES);

  RespondResult(0, InitializeResult("2025-11-25"), Format::kSse,
                {{"Mcp-Session-Id", kSessionId}});

  // `notifications/initialized` is sent alone, before any other request.
  EXPECT_EQ(Body(1),
            ParseJsonDict(
                R"({"jsonrpc":"2.0","method":"notifications/initialized"})"));
  EXPECT_EQ(RequestCount(), 2u);
  EXPECT_EQ(Header(1, "MCP-Protocol-Version"), "2025-11-25");
  EXPECT_EQ(Header(1, "Mcp-Session-Id"), kSessionId);
  EXPECT_FALSE(future.IsReady());
  RespondAccepted(1);

  EXPECT_EQ(Method(2), "tools/list");
  EXPECT_FALSE(Body(2).contains("params"));
  EXPECT_EQ(Header(2, "MCP-Protocol-Version"), "2025-11-25");
  EXPECT_EQ(Header(2, "Mcp-Session-Id"), kSessionId);
  RespondResult(2, ToolsListResult({"search"}));

  ToolsResult result = Take(future);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ((*result)[0].name, "search");
  EXPECT_EQ((*result)[0].input_schema, ParseJsonDict(R"({"type":"object"})"));
  EXPECT_EQ(client_->state(), MCPClient::State::kReady);
  EXPECT_FALSE(client_->last_error());
}

TEST_F(MCPClientTest, AcceptsSupportedProtocolVersions) {
  for (const char* version : {"2025-11-25", "2025-06-18", "2025-03-26"}) {
    for (Format format : {Format::kJson, Format::kSse}) {
      SCOPED_TRACE(
          base::StrCat({version, format == Format::kJson ? " JSON" : " SSE"}));
      CreateClient();
      const size_t start = RequestCount();
      base::test::TestFuture<ToolsResult> future;
      client_->GetTools(future.GetCallback());

      const size_t list =
          CompleteHandshake(start, "session-x", version, format);
      // The session id is captured from either kind of response.
      EXPECT_EQ(Header(list, "MCP-Protocol-Version"), version);
      EXPECT_EQ(Header(list, "Mcp-Session-Id"), "session-x");
      RespondResult(list, ToolsListResult({"a"}), format);
      EXPECT_EQ(ToolNames(Take(future)), std::vector<std::string>({"a"}));
    }
  }
}

TEST_F(MCPClientTest, UnsupportedProtocolVersion) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  RespondResult(0, InitializeResult("2099-01-01"), Format::kSse,
                {{"Mcp-Session-Id", kSessionId}});

  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kUnsupportedProtocolVersion);
  EXPECT_THAT(result.error().message, HasSubstr(kServerName));
  EXPECT_THAT(result.error().message, HasSubstr("\"2099-01-01\""));
  EXPECT_EQ(client_->state(), MCPClient::State::kFailed);
  EXPECT_EQ(client_->last_error(), result.error());

  // The session is ended rather than initialized.
  EXPECT_EQ(Request(1).method, "DELETE");
  EXPECT_EQ(Header(1, "Mcp-Session-Id"), kSessionId);
  EXPECT_FALSE(Request(1).headers.HasHeader("MCP-Protocol-Version"));
  EXPECT_EQ(RequestCount(), 2u);
}

TEST_F(MCPClientTest, UnsupportedProtocolVersionIsSanitized) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  RespondResult(
      0, InitializeResult(base::StrCat({R"(v\u0007)", std::string(50, '9')})));
  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_THAT(result.error().message,
              HasSubstr(base::StrCat({"\"v?", std::string(30, '9'), "\""})));
}

TEST_F(MCPClientTest, ServerWithoutToolsCapabilityFails) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  RespondResult(0, InitializeResult("2025-11-25", /*with_tools=*/false));

  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kToolsNotSupported);
  EXPECT_EQ(client_->state(), MCPClient::State::kFailed);
  // Neither `notifications/initialized` nor a DELETE without a session.
  EXPECT_EQ(RequestCount(), 1u);
}

TEST_F(MCPClientTest, SessionIdNeverSentWhenAbsent) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t list = CompleteHandshake(0, std::nullopt);
  const size_t indices[] = {1u, list};
  for (size_t index : indices) {
    EXPECT_FALSE(Request(index).headers.HasHeader("Mcp-Session-Id"));
    EXPECT_EQ(Header(index, "MCP-Protocol-Version"), "2025-11-25");
  }
  RespondResult(list, ToolsListResult({}));
  EXPECT_TRUE(Take(future).has_value());

  // No session to end.
  client_.reset();
  EXPECT_EQ(RequestCount(), list + 1);
}

TEST_F(MCPClientTest, SessionHeadersOnEveryRequestAndDeleteOnDestruction) {
  CreateClient("token");
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("echo", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  Respond(
      call, net::HTTP_OK, "text/event-stream",
      base::StrCat({MakeSSEEvent(R"({"jsonrpc":"2.0","id":1,"method":"ping"})"),
                    MakeSSEEvent(ResultMessage(call, kTextContentResult))}));
  EXPECT_TRUE(Take(future).has_value());
  // The ping reply.
  EXPECT_EQ(Body(call + 1),
            ParseJsonDict(R"({"jsonrpc":"2.0","id":1,"result":{}})"));
  RespondAccepted(call + 1);

  client_.reset();
  EXPECT_EQ(Request(call + 2).method, "DELETE");
  for (size_t index = 1; index <= call + 2; ++index) {
    SCOPED_TRACE(index);
    EXPECT_EQ(Header(index, "Mcp-Session-Id"), kSessionId);
    EXPECT_EQ(Header(index, "MCP-Protocol-Version"), "2025-11-25");
    EXPECT_EQ(Header(index, "Authorization"), "Bearer token");
  }
  EXPECT_EQ(RequestCount(), call + 3);
}

TEST_F(MCPClientTest, SessionExpiryReinitializesAndRetries) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t list = CompleteHandshake(0);
  Respond(list, net::HTTP_NOT_FOUND, "text/plain", "Session not found");

  // A new session, started without the old id.
  EXPECT_EQ(Method(list + 1), "initialize");
  EXPECT_FALSE(Request(list + 1).headers.HasHeader("Mcp-Session-Id"));
  EXPECT_FALSE(Request(list + 1).headers.HasHeader("MCP-Protocol-Version"));
  const size_t retry = CompleteHandshake(list + 1, "session-2");

  EXPECT_EQ(Method(retry), "tools/list");
  EXPECT_EQ(Id(retry), Id(list));
  EXPECT_EQ(Header(retry, "Mcp-Session-Id"), "session-2");
  RespondResult(retry, ToolsListResult({"a"}));
  EXPECT_EQ(ToolNames(Take(future)), std::vector<std::string>({"a"}));
  EXPECT_EQ(client_->state(), MCPClient::State::kReady);
}

TEST_F(MCPClientTest, SessionExpiryRetriesOnlyOnce) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t list = CompleteHandshake(0);
  Respond(list, net::HTTP_NOT_FOUND, "text/plain", "");
  const size_t retry = CompleteHandshake(list + 1, "session-2");
  EXPECT_EQ(Method(retry), "tools/list");
  Respond(retry, net::HTTP_NOT_FOUND, "text/plain", "");

  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kSessionExpired);
  EXPECT_EQ(RequestCount(), retry + 1);
}

TEST_F(MCPClientTest, ConcurrentSessionExpiryReinitializesOnce) {
  base::test::TestFuture<CallResult> first_future;
  base::test::TestFuture<CallResult> second_future;
  base::OnceClosure cancel_first =
      client_->CallTool("a", base::DictValue(), first_future.GetCallback());
  base::OnceClosure cancel_second =
      client_->CallTool("b", base::DictValue(), second_future.GetCallback());
  const size_t first = CompleteHandshake(0);
  EXPECT_EQ(Method(first), "tools/call");
  EXPECT_EQ(Method(first + 1), "tools/call");
  Respond(first, net::HTTP_NOT_FOUND, "text/plain", "");
  Respond(first + 1, net::HTTP_NOT_FOUND, "text/plain", "");

  const size_t retries = CompleteHandshake(first + 2, "session-2");
  EXPECT_EQ(Method(retries), "tools/call");
  EXPECT_EQ(Method(retries + 1), "tools/call");
  EXPECT_EQ(RequestCount(), retries + 2);
  RespondResult(retries, kTextContentResult);
  RespondResult(retries + 1, kTextContentResult);
  EXPECT_TRUE(Take(first_future).has_value());
  EXPECT_TRUE(Take(second_future).has_value());
}

TEST_F(MCPClientTest, StreamEndingWithoutResponseIsNotRetried) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("send_email", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  Respond(call, net::HTTP_OK, "text/event-stream",
          MakeSSEEvent(R"({"jsonrpc":"2.0","method":"notifications/message",
                       "params":{"level":"info","data":"working"}})"));

  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kNoResponse);
  EXPECT_THAT(result.error().message, HasSubstr(kServerName));
  EXPECT_EQ(RequestCount(), call + 1);
}

TEST_F(MCPClientTest, AnswersServerRequestsOnStream) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("echo", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  Respond(
      call, net::HTTP_OK, "text/event-stream",
      base::StrCat(
          {MakeSSEEvent(R"({"jsonrpc":"2.0","id":"ping-1","method":"ping"})"),
           MakeSSEEvent(R"({"jsonrpc":"2.0","id":7,
                            "method":"sampling/createMessage","params":{}})"),
           MakeSSEEvent(ResultMessage(call, kTextContentResult))}));

  CallResult result = Take(future);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->content.size(), 1u);
  EXPECT_EQ(std::get<MCPTextContent>(result->content[0]).text, "done");

  EXPECT_EQ(Body(call + 1),
            ParseJsonDict(R"({"jsonrpc":"2.0","id":"ping-1","result":{}})"));
  EXPECT_EQ(Header(call + 1, "Mcp-Session-Id"), kSessionId);
  EXPECT_EQ(Body(call + 2), ParseJsonDict(R"({"jsonrpc":"2.0","id":7,
      "error":{"code":-32601,"message":"Method not found"}})"));
  EXPECT_EQ(Header(call + 2, "Mcp-Session-Id"), kSessionId);
  RespondAccepted(call + 1);
  RespondAccepted(call + 2);
}

TEST_F(MCPClientTest, CancelToolCallNotifiesServer) {
  base::OnceClosure cancel =
      client_->CallTool("slow", base::DictValue(),
                        base::BindOnce([](CallResult) { ADD_FAILURE(); }));
  const size_t call = CompleteHandshake(0);
  EXPECT_EQ(Method(call), "tools/call");
  const int call_id = Id(call);

  std::move(cancel).Run();
  EXPECT_EQ(Body(call + 1),
            ParseJsonDict(base::StrCat(
                {R"({"jsonrpc":"2.0","method":"notifications/cancelled",
                     "params":{"requestId":)",
                 base::NumberToString(call_id),
                 R"(,"reason":"The client cancelled the request."}})"})));
  EXPECT_EQ(Header(call + 1, "Mcp-Session-Id"), kSessionId);
  EXPECT_EQ(Header(call + 1, "MCP-Protocol-Version"), "2025-11-25");
  // The call's own connection is dropped.
  RunReadyTasks();
  EXPECT_FALSE(Pending(call).client.is_connected());
  RespondAccepted(call + 1);
  task_environment_.FastForwardBy(kMCPMaxRequestTimeout);
}

TEST_F(MCPClientTest, CancelBeforeSendingSendsNothing) {
  base::OnceClosure cancel =
      client_->CallTool("slow", base::DictValue(),
                        base::BindOnce([](CallResult) { ADD_FAILURE(); }));
  EXPECT_EQ(Method(0), "initialize");
  std::move(cancel).Run();
  const size_t next = CompleteHandshake(0);

  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  // Neither the call nor a cancellation was sent.
  EXPECT_EQ(Method(next), "tools/list");
  RespondResult(next, ToolsListResult({}));
  EXPECT_TRUE(Take(future).has_value());
}

TEST_F(MCPClientTest, CancelAfterCompletionDoesNothing) {
  base::test::TestFuture<CallResult> call_future;
  base::OnceClosure cancel =
      client_->CallTool("echo", base::DictValue(), call_future.GetCallback());
  const size_t call = CompleteHandshake(0);
  RespondResult(call, kTextContentResult);
  EXPECT_TRUE(Take(call_future).has_value());
  std::move(cancel).Run();

  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  EXPECT_EQ(Method(call + 1), "tools/list");
  RespondResult(call + 1, ToolsListResult({}));
  EXPECT_TRUE(Take(future).has_value());
}

TEST_F(MCPClientTest, CallToolRequestAndResults) {
  base::test::TestFuture<CallResult> tool_error_future;
  base::test::TestFuture<CallResult> server_error_future;
  base::test::TestFuture<CallResult> malformed_future;
  base::OnceClosure cancel_tool_error =
      client_->CallTool("echo", ParseJsonDict(R"({"text":"hi"})"),
                        tool_error_future.GetCallback());
  base::OnceClosure cancel_server_error = client_->CallTool(
      "echo", base::DictValue(), server_error_future.GetCallback());
  base::OnceClosure cancel_malformed = client_->CallTool(
      "echo", base::DictValue(), malformed_future.GetCallback());
  const size_t call = CompleteHandshake(0);

  const int call_id = Id(call);
  EXPECT_EQ(
      *Body(call).FindDict("params"),
      ParseJsonDict(base::StrCat({R"({"name":"echo","arguments":{"text":"hi"},
                     "_meta":{"progressToken":)",
                                  base::NumberToString(call_id), "}}"})));

  // The tool failed, which the model is told about.
  RespondResult(
      call,
      R"({"content":[{"type":"text","text":"no such file"}],"isError":true})");
  // The server rejected the call.
  Respond(call + 1, net::HTTP_OK, "application/json",
          base::StrCat(
              {R"({"jsonrpc":"2.0","id":)", base::NumberToString(Id(call + 1)),
               R"(,"error":{"code":-32602,"message":"Unknown tool"}})"}));
  RespondResult(call + 2, R"({"content":"not a list"})");

  CallResult result = Take(tool_error_future);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->is_error);
  ASSERT_EQ(result->content.size(), 1u);
  EXPECT_EQ(std::get<MCPTextContent>(result->content[0]).text, "no such file");

  result = Take(server_error_future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kServerError);
  EXPECT_THAT(result.error().message, HasSubstr("Unknown tool"));

  result = Take(malformed_future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kInvalidResponse);
}

TEST_F(MCPClientTest, ToolsListPagination) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t page = CompleteHandshake(0);
  RespondResult(page, ToolsListResult({"a", "b"}, "cursor-1"));
  base::DictValue body = Body(page + 1);
  EXPECT_EQ(*body.FindString("method"), "tools/list");
  EXPECT_EQ(*body.FindStringByDottedPath("params.cursor"), "cursor-1");
  RespondResult(page + 1, ToolsListResult({"c"}, "cursor-2"));
  body = Body(page + 2);
  EXPECT_EQ(*body.FindStringByDottedPath("params.cursor"), "cursor-2");
  RespondResult(page + 2, ToolsListResult({"d"}));

  EXPECT_EQ(ToolNames(Take(future)),
            std::vector<std::string>({"a", "b", "c", "d"}));
}

TEST_F(MCPClientTest, ToolsListPageCap) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t page = CompleteHandshake(0);
  std::vector<std::string> expected;
  for (size_t i = 0; i < kMCPMaxToolsListPages; ++i) {
    expected.push_back(base::StrCat({"tool", base::NumberToString(i)}));
    RespondResult(page + i, ToolsListResult({expected.back()}, "more"));
  }
  EXPECT_EQ(ToolNames(Take(future)), expected);
  EXPECT_EQ(RequestCount(), page + kMCPMaxToolsListPages);
}

TEST_F(MCPClientTest, ToolsPerServerCap) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t page = CompleteHandshake(0);
  std::vector<std::string> names;
  for (size_t i = 0; i < kMCPMaxToolsPerServer + 5; ++i) {
    names.push_back(base::StrCat({"tool", base::NumberToString(i)}));
  }
  RespondResult(page, ToolsListResult(names, "more"));

  ToolsResult result = Take(future);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), kMCPMaxToolsPerServer);
  // No further page once the cap is reached.
  EXPECT_EQ(RequestCount(), page + 1);
}

TEST_F(MCPClientTest, ToolsListSkipsMalformedAndDuplicateTools) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t page = CompleteHandshake(0);
  RespondResult(page, R"({"tools":[
      {"name":"a","inputSchema":{}},
      {"name":"a","inputSchema":{"type":"object"}},
      {"name":"b"},
      "c",
      {"name":"d","inputSchema":{},"description":5},
      {"name":"e","inputSchema":{}}
  ]})");
  EXPECT_EQ(ToolNames(Take(future)), std::vector<std::string>({"a", "e"}));
}

TEST_F(MCPClientTest, ToolsListWithoutToolsIsInvalid) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  RespondResult(CompleteHandshake(0), R"({"tools":{}})");
  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kInvalidResponse);
}

TEST_F(MCPClientTest, ToolsCacheTtl) {
  base::test::TestFuture<ToolsResult> first;
  client_->GetTools(first.GetCallback());
  const size_t page = CompleteHandshake(0);
  RespondResult(page, ToolsListResult({"a"}));
  EXPECT_EQ(ToolNames(Take(first)), std::vector<std::string>({"a"}));

  base::test::TestFuture<ToolsResult> cached;
  client_->GetTools(cached.GetCallback());
  // Never synchronous, even from the cache.
  EXPECT_FALSE(cached.IsReady());
  EXPECT_EQ(ToolNames(Take(cached)), std::vector<std::string>({"a"}));
  EXPECT_EQ(RequestCount(), page + 1);

  task_environment_.FastForwardBy(kMCPToolsCacheTtl);
  base::test::TestFuture<ToolsResult> refreshed;
  client_->GetTools(refreshed.GetCallback());
  EXPECT_EQ(Method(page + 1), "tools/list");
  RespondResult(page + 1, ToolsListResult({"b"}));
  EXPECT_EQ(ToolNames(Take(refreshed)), std::vector<std::string>({"b"}));
}

TEST_F(MCPClientTest, QueuesConcurrentCallsBehindOneHandshake) {
  base::test::TestFuture<ToolsResult> tools_1;
  base::test::TestFuture<ToolsResult> tools_2;
  base::test::TestFuture<CallResult> call_1;
  base::test::TestFuture<CallResult> call_2;
  client_->GetTools(tools_1.GetCallback());
  client_->GetTools(tools_2.GetCallback());
  base::OnceClosure cancel_1 =
      client_->CallTool("a", base::DictValue(), call_1.GetCallback());
  base::OnceClosure cancel_2 =
      client_->CallTool("b", base::DictValue(), call_2.GetCallback());

  EXPECT_EQ(Method(0), "initialize");
  EXPECT_EQ(RequestCount(), 1u);
  const size_t next = CompleteHandshake(0);

  // One `tools/list` serves both GetTools() calls.
  EXPECT_EQ(Method(next), "tools/list");
  EXPECT_EQ(Method(next + 1), "tools/call");
  EXPECT_EQ(Method(next + 2), "tools/call");
  EXPECT_EQ(RequestCount(), next + 3);
  RespondResult(next, ToolsListResult({"a", "b"}));
  RespondResult(next + 1, kTextContentResult);
  RespondResult(next + 2, kTextContentResult, Format::kJson);

  EXPECT_EQ(ToolNames(Take(tools_1)), std::vector<std::string>({"a", "b"}));
  EXPECT_EQ(ToolNames(Take(tools_2)), std::vector<std::string>({"a", "b"}));
  EXPECT_TRUE(Take(call_1).has_value());
  EXPECT_TRUE(Take(call_2).has_value());
}

TEST_F(MCPClientTest, HttpSseOnlyServer) {
  for (int status : {net::HTTP_BAD_REQUEST, net::HTTP_NOT_FOUND,
                     net::HTTP_METHOD_NOT_ALLOWED}) {
    SCOPED_TRACE(status);
    CreateClient();
    const size_t start = RequestCount();
    base::test::TestFuture<ToolsResult> future;
    client_->GetTools(future.GetCallback());
    Respond(start, status, "text/plain", "Not Found");

    ToolsResult result = Take(future);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, MCPErrorCode::kStreamableHttpNotSupported);
    EXPECT_THAT(result.error().message, HasSubstr("/mcp"));
    EXPECT_THAT(result.error().message, HasSubstr(kServerName));
    EXPECT_EQ(client_->state(), MCPClient::State::kFailed);
  }
}

TEST_F(MCPClientTest, InitializeJsonRpcErrorBodyIsSurfaced) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  Respond(0, net::HTTP_BAD_REQUEST, "application/json",
          R"({"jsonrpc":"2.0","id":null,
              "error":{"code":-32600,"message":"Invalid request"}})");

  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kServerError);
  EXPECT_THAT(result.error().message, HasSubstr("Invalid request"));
}

TEST_F(MCPClientTest, AuthenticationErrors) {
  for (const char* token : {"", "token"}) {
    for (int status : {net::HTTP_UNAUTHORIZED, net::HTTP_FORBIDDEN}) {
      SCOPED_TRACE(base::StrCat({token, " ", base::NumberToString(status)}));
      std::optional<std::string> configured_token;
      if (*token) {
        configured_token = token;
      }
      CreateClient(configured_token);
      const size_t start = RequestCount();
      base::test::TestFuture<ToolsResult> future;
      client_->GetTools(future.GetCallback());
      if (*token) {
        EXPECT_EQ(Header(start, "Authorization"), "Bearer token");
      }
      // The status decides, even with a JSON-RPC error body.
      Respond(start, status, "application/json",
              R"({"jsonrpc":"2.0","id":null,
                  "error":{"code":-32001,"message":"Unauthorized"}})");

      ToolsResult result = Take(future);
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error().code, *token ? MCPErrorCode::kAuthRejected
                                            : MCPErrorCode::kAuthRequired);
      EXPECT_THAT(result.error().message, HasSubstr("access token"));
    }
  }
}

TEST_F(MCPClientTest, ToolCallTimeout) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("slow", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  EXPECT_EQ(Method(call), "tools/call");
  task_environment_.FastForwardBy(kMCPRequestTimeout - base::Seconds(1));
  EXPECT_FALSE(future.IsReady());
  task_environment_.FastForwardBy(base::Seconds(1));

  ASSERT_TRUE(future.IsReady());
  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kTimeout);
  // The server is told to stop.
  const base::DictValue cancelled = Body(call + 1);
  EXPECT_EQ(*cancelled.FindString("method"), "notifications/cancelled");
  EXPECT_EQ(cancelled.FindIntByDottedPath("params.requestId"), Id(call));
}

TEST_F(MCPClientTest, ProgressExtendsTimeoutUpToMaximum) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("slow", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  const int token =
      *Body(call).FindIntByDottedPath("params._meta.progressToken");
  mojo::ScopedDataPipeProducerHandle stream = StartStream(call);
  const std::string progress = MakeSSEEvent(
      base::StrCat({R"({"jsonrpc":"2.0","method":"notifications/progress",
           "params":{"progressToken":)",
                    base::NumberToString(token), R"(,"progress":1}})"}));

  const base::TimeDelta interval = kMCPRequestTimeout - base::Seconds(10);
  base::TimeDelta elapsed;
  while (elapsed + interval < kMCPMaxRequestTimeout) {
    ASSERT_TRUE(mojo::BlockingCopyFromString(progress, stream));
    task_environment_.FastForwardBy(interval);
    elapsed += interval;
    ASSERT_FALSE(future.IsReady()) << elapsed;
  }
  // Progress can't extend the call past the maximum.
  ASSERT_TRUE(mojo::BlockingCopyFromString(progress, stream));
  task_environment_.FastForwardBy(kMCPMaxRequestTimeout - elapsed -
                                  base::Seconds(1));
  EXPECT_FALSE(future.IsReady());
  task_environment_.FastForwardBy(base::Seconds(1));
  ASSERT_TRUE(future.IsReady());
  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kTimeout);
}

TEST_F(MCPClientTest, ResponseTooLarge) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("big", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  RespondResult(
      call, base::StrCat({R"({"content":[{"type":"text","text":")",
                          std::string(kMCPMaxResponseBytes, 'a'), R"("}]})"}));

  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kResponseTooLarge);
  EXPECT_THAT(result.error().message, HasSubstr("4 MB"));
}

// A response that arrives before its stream crosses the size cap completes the
// call, and the abort that follows it is ignored.
TEST_F(MCPClientTest, ResponseBeforeStreamCrossesCap) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("echo", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  const std::string event =
      MakeSSEEvent(ResultMessage(call, kTextContentResult));
  // Padded with a comment, so that the response fills the cap exactly.
  const std::string response = base::StrCat(
      {":", std::string(kMCPMaxResponseBytes - event.size() - 2, ' '), "\n",
       event});
  ASSERT_EQ(response.size(), kMCPMaxResponseBytes);

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  ASSERT_EQ(mojo::CreateDataPipe(response.size(), producer, consumer),
            MOJO_RESULT_OK);
  Pending(call).client->OnReceiveResponse(
      MakeMCPResponseHead(net::HTTP_OK, "text/event-stream"),
      std::move(consumer), std::nullopt);
  RunReadyTasks();

  // Runs the main thread until `producer` has `signal`. Unlike
  // `RunReadyTasks()`, this doesn't run thread pool tasks held by a fence.
  auto wait_for = [&](MojoHandleSignals signal) {
    base::RunLoop run_loop;
    mojo::SimpleWatcher watcher(FROM_HERE,
                                mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC);
    watcher.Watch(producer.get(), signal, MOJO_WATCH_CONDITION_SATISFIED,
                  base::BindLambdaForTesting(
                      [&](MojoResult, const mojo::HandleSignalsState&) {
                        run_loop.Quit();
                      }));
    run_loop.Run();
  };
  {
    // Holds the response's parse until the stream has crossed the cap.
    base::test::TaskEnvironment::ParallelExecutionFence fence;
    ASSERT_EQ(producer->WriteAllData(base::as_byte_span(response)),
              MOJO_RESULT_OK);
    // Writable again once the loader has read the response.
    wait_for(MOJO_HANDLE_SIGNAL_WRITABLE);
    ASSERT_EQ(producer->WriteAllData(base::byte_span_from_cstring("\n")),
              MOJO_RESULT_OK);
    // The loader is aborted.
    wait_for(MOJO_HANDLE_SIGNAL_PEER_CLOSED);
    EXPECT_FALSE(future.IsReady());
  }

  CallResult result = Take(future);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->content.size(), 1u);
  EXPECT_EQ(std::get<MCPTextContent>(result->content[0]).text, "done");
  // Nothing more is sent, such as a cancellation.
  EXPECT_EQ(RequestCount(), call + 1);
}

TEST_F(MCPClientTest, InvalidConfig) {
  struct {
    const char* endpoint;
    std::optional<std::string> token;
    MCPErrorCode code;
  } const kCases[] = {
      {"http://mcp.example.com/mcp", std::nullopt,
       MCPErrorCode::kInvalidEndpoint},
      {"https://user:pass@mcp.example.com/mcp", std::nullopt,
       MCPErrorCode::kInvalidEndpoint},
      {kEndpoint, "bad\ntoken", MCPErrorCode::kInvalidToken},
  };
  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.endpoint);
    base::expected<std::unique_ptr<MCPClient>, MCPError> client =
        MCPClient::Create(MCPServerConfig(GURL(test_case.endpoint),
                                          test_case.token, kServerName),
                          url_loader_factory_.GetSafeWeakWrapper());
    ASSERT_FALSE(client.has_value());
    EXPECT_EQ(client.error().code, test_case.code);
    EXPECT_THAT(client.error().message, HasSubstr(kServerName));
  }
  EXPECT_EQ(RequestCount(), 0u);
}

TEST_F(MCPClientTest, FailedInitializeIsRetriedOnNextCall) {
  base::test::TestFuture<ToolsResult> failed;
  client_->GetTools(failed.GetCallback());
  url_loader_factory_.SimulateResponseWithoutRemovingFromPendingList(
      &Pending(0), MakeMCPResponseHead(net::HTTP_OK, ""), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));
  ToolsResult result = Take(failed);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kNetworkError);
  EXPECT_THAT(result.error().message, HasSubstr("ERR_CONNECTION_REFUSED"));
  EXPECT_EQ(client_->state(), MCPClient::State::kFailed);
  EXPECT_EQ(client_->last_error(), result.error());

  base::test::TestFuture<ToolsResult> retried;
  client_->GetTools(retried.GetCallback());
  const size_t list = CompleteHandshake(1);
  RespondResult(list, ToolsListResult({"a"}));
  EXPECT_EQ(ToolNames(Take(retried)), std::vector<std::string>({"a"}));
  EXPECT_EQ(client_->state(), MCPClient::State::kReady);
  EXPECT_FALSE(client_->last_error());
}

TEST_F(MCPClientTest, NoCallbacksAfterDestruction) {
  base::test::TestFuture<ToolsResult> cached;
  client_->GetTools(cached.GetCallback());
  const size_t list = CompleteHandshake(0);
  RespondResult(list, ToolsListResult({"a"}));
  EXPECT_TRUE(Take(cached).has_value());

  // A cache hit, whose reply is posted, and a tool call on the wire.
  client_->GetTools(base::BindOnce([](ToolsResult) { ADD_FAILURE(); }));
  base::OnceClosure cancel =
      client_->CallTool("slow", base::DictValue(),
                        base::BindOnce([](CallResult) { ADD_FAILURE(); }));
  client_.reset();

  // Cancelling after destruction does nothing more.
  std::move(cancel).Run();
  EXPECT_EQ(Method(list + 1), "tools/call");
  EXPECT_EQ(Method(list + 2), "notifications/cancelled");
  EXPECT_EQ(Request(list + 3).method, "DELETE");
  EXPECT_EQ(RequestCount(), list + 4);
  task_environment_.FastForwardBy(kMCPMaxRequestTimeout);
}

TEST_F(MCPClientTest, DestructionCancelsCallsInFlight) {
  for (bool with_session : {true, false}) {
    SCOPED_TRACE(with_session);
    CreateClient();
    const size_t start = RequestCount();
    base::OnceClosure cancel =
        client_->CallTool("slow", base::DictValue(),
                          base::BindOnce([](CallResult) { ADD_FAILURE(); }));
    const size_t call = CompleteHandshake(
        start, with_session ? std::make_optional<std::string>(kSessionId)
                            : std::nullopt);
    EXPECT_EQ(Method(call), "tools/call");
    const int call_id = Id(call);
    client_.reset();

    const base::DictValue cancelled = Body(call + 1);
    EXPECT_EQ(*cancelled.FindString("method"), "notifications/cancelled");
    EXPECT_EQ(cancelled.FindIntByDottedPath("params.requestId"), call_id);
    EXPECT_EQ(*cancelled.FindStringByDottedPath("params.reason"),
              "The client shut down.");
    EXPECT_EQ(Header(call + 1, "MCP-Protocol-Version"), "2025-11-25");
    EXPECT_EQ(Header(call + 1, "Mcp-Session-Id"),
              with_session ? std::make_optional<std::string>(kSessionId)
                           : std::nullopt);
    // It outlives the client.
    EXPECT_TRUE(Pending(call + 1).client.is_connected());
    if (with_session) {
      EXPECT_EQ(Request(call + 2).method, "DELETE");
    }
    EXPECT_EQ(RequestCount(), call + (with_session ? 3 : 2));
  }
}

TEST_F(MCPClientTest, CancellationSurvivesDestruction) {
  base::OnceClosure cancel =
      client_->CallTool("slow", base::DictValue(),
                        base::BindOnce([](CallResult) { ADD_FAILURE(); }));
  const size_t call = CompleteHandshake(0);
  EXPECT_EQ(Method(call), "tools/call");
  std::move(cancel).Run();
  client_.reset();

  EXPECT_EQ(Method(call + 1), "notifications/cancelled");
  EXPECT_TRUE(Pending(call + 1).client.is_connected());
  // Only the session is ended; the cancelled call isn't cancelled again.
  EXPECT_EQ(Request(call + 2).method, "DELETE");
  EXPECT_EQ(RequestCount(), call + 3);
}

TEST_F(MCPClientTest, DestructionDuringInitializeCancelsNothing) {
  client_->GetTools(base::BindOnce([](ToolsResult) { ADD_FAILURE(); }));
  EXPECT_EQ(Method(0), "initialize");
  client_.reset();
  EXPECT_EQ(RequestCount(), 1u);
}

TEST_F(MCPClientTest, CancellationAfterSessionExpiryUsesOriginalSession) {
  base::test::TestFuture<CallResult> first;
  base::OnceClosure cancel_first =
      client_->CallTool("a", base::DictValue(), first.GetCallback());
  base::OnceClosure cancel_second =
      client_->CallTool("b", base::DictValue(),
                        base::BindOnce([](CallResult) { ADD_FAILURE(); }));
  const size_t calls = CompleteHandshake(0);
  EXPECT_EQ(Method(calls + 1), "tools/call");
  // The first call's session expired, so the client re-initializes.
  Respond(calls, net::HTTP_NOT_FOUND, "text/plain", "");
  EXPECT_EQ(Method(calls + 2), "initialize");

  // The second call, still on the old session, is cancelled meanwhile.
  std::move(cancel_second).Run();
  EXPECT_EQ(Method(calls + 3), "notifications/cancelled");
  EXPECT_EQ(Header(calls + 3, "Mcp-Session-Id"), kSessionId);
  EXPECT_EQ(Header(calls + 3, "MCP-Protocol-Version"), "2025-11-25");
}

TEST_F(MCPClientTest, InitializeStreamLeftOpen) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  mojo::ScopedDataPipeProducerHandle stream =
      StartStream(0, {{"Mcp-Session-Id", kSessionId}});
  ASSERT_TRUE(mojo::BlockingCopyFromString(
      MakeSSEEvent(ResultMessage(0, InitializeResult("2025-11-25"))), stream));

  // The handshake goes on without the stream ending, which is dropped.
  EXPECT_EQ(Method(1), "notifications/initialized");
  EXPECT_EQ(Header(1, "Mcp-Session-Id"), kSessionId);
  EXPECT_FALSE(Pending(0).client.is_connected());
  RespondAccepted(1);
  EXPECT_EQ(Header(2, "Mcp-Session-Id"), kSessionId);
  RespondResult(2, ToolsListResult({"a"}));
  EXPECT_EQ(ToolNames(Take(future)), std::vector<std::string>({"a"}));
}

TEST_F(MCPClientTest, PingDuringInitializeCarriesNewSession) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  mojo::ScopedDataPipeProducerHandle stream =
      StartStream(0, {{"Mcp-Session-Id", kSessionId}});
  ASSERT_TRUE(mojo::BlockingCopyFromString(
      MakeSSEEvent(R"({"jsonrpc":"2.0","id":"p1","method":"ping"})"), stream));

  EXPECT_EQ(Body(1),
            ParseJsonDict(R"({"jsonrpc":"2.0","id":"p1","result":{}})"));
  EXPECT_EQ(Header(1, "Mcp-Session-Id"), kSessionId);
  // No version has been negotiated yet.
  EXPECT_FALSE(Request(1).headers.HasHeader("MCP-Protocol-Version"));
  RespondAccepted(1);

  ASSERT_TRUE(mojo::BlockingCopyFromString(
      MakeSSEEvent(ResultMessage(0, InitializeResult("2025-11-25"))), stream));
  EXPECT_EQ(Method(2), "notifications/initialized");
  EXPECT_EQ(Header(2, "Mcp-Session-Id"), kSessionId);
}

TEST_F(MCPClientTest, InitializedNotificationAcceptsEmpty200) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  RespondResult(0, InitializeResult("2025-11-25"));
  EXPECT_EQ(Method(1), "notifications/initialized");
  Respond(1, net::HTTP_OK, "", "");
  EXPECT_EQ(Method(2), "tools/list");
  RespondResult(2, ToolsListResult({}));
  EXPECT_TRUE(Take(future).has_value());
}

TEST_F(MCPClientTest, InitializeTimeoutFailsQueuedCalls) {
  base::test::TestFuture<ToolsResult> tools;
  base::test::TestFuture<CallResult> call;
  client_->GetTools(tools.GetCallback());
  base::OnceClosure cancel =
      client_->CallTool("a", base::DictValue(), call.GetCallback());
  EXPECT_EQ(Method(0), "initialize");
  task_environment_.FastForwardBy(kMCPRequestTimeout);

  ASSERT_TRUE(tools.IsReady());
  ASSERT_TRUE(call.IsReady());
  ToolsResult tools_result = Take(tools);
  ASSERT_FALSE(tools_result.has_value());
  EXPECT_EQ(tools_result.error().code, MCPErrorCode::kTimeout);
  CallResult call_result = Take(call);
  ASSERT_FALSE(call_result.has_value());
  EXPECT_EQ(call_result.error().code, MCPErrorCode::kTimeout);
  EXPECT_EQ(client_->state(), MCPClient::State::kFailed);
  EXPECT_EQ(client_->last_error()->code, MCPErrorCode::kTimeout);
  // `initialize` is never cancelled.
  EXPECT_EQ(RequestCount(), 1u);
}

TEST_F(MCPClientTest, DestroyedByCallbackWhileFailingQueuedCalls) {
  size_t callbacks = 0;
  base::OnceClosure cancel_first = client_->CallTool(
      "a", base::DictValue(), base::BindLambdaForTesting([&](CallResult) {
        ++callbacks;
        client_.reset();
      }));
  base::OnceClosure cancel_second =
      client_->CallTool("b", base::DictValue(),
                        base::BindOnce([](CallResult) { ADD_FAILURE(); }));
  Respond(0, net::HTTP_METHOD_NOT_ALLOWED, "text/plain", "");
  RunReadyTasks();
  EXPECT_EQ(callbacks, 1u);
  EXPECT_FALSE(client_);
}

TEST_F(MCPClientTest, DestroyedByCallbackWhileFinishingGetTools) {
  size_t callbacks = 0;
  client_->GetTools(base::BindLambdaForTesting([&](ToolsResult result) {
    EXPECT_TRUE(result.has_value());
    ++callbacks;
    client_.reset();
  }));
  client_->GetTools(base::BindOnce([](ToolsResult) { ADD_FAILURE(); }));
  const size_t list = CompleteHandshake(0);
  RespondResult(list, ToolsListResult({"a"}));
  RunReadyTasks();
  EXPECT_EQ(callbacks, 1u);
  EXPECT_FALSE(client_);
}

TEST_F(MCPClientTest, NetworkErrorMidStreamIsNotRetried) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("send_email", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  mojo::ScopedDataPipeProducerHandle stream = StartStream(call);
  ASSERT_TRUE(mojo::BlockingCopyFromString(
      MakeSSEEvent(R"({"jsonrpc":"2.0","method":"notifications/message",
                   "params":{"level":"info","data":"sending"}})"),
      stream));
  RunReadyTasks();
  stream.reset();
  Pending(call).client->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_RESET));

  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kNetworkError);
  EXPECT_EQ(RequestCount(), call + 1);
}

TEST_F(MCPClientTest, NotFoundWithoutSessionIsNotRetried) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  const size_t list = CompleteHandshake(0, std::nullopt);
  Respond(list, net::HTTP_NOT_FOUND, "text/plain", "Not Found");

  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kHttpClientError);
  EXPECT_THAT(result.error().message, HasSubstr("404"));
  EXPECT_EQ(RequestCount(), list + 1);
  EXPECT_EQ(client_->state(), MCPClient::State::kReady);
}

// Only a 4xx that doesn't ask to try again later says the request is wrong.
TEST_F(MCPClientTest, HttpErrorsThatMayPass) {
  size_t index = 0;
  for (auto [status, code] :
       {std::pair(net::HTTP_GONE, MCPErrorCode::kHttpClientError),
        std::pair(net::HTTP_REQUEST_TIMEOUT, MCPErrorCode::kHttpError),
        std::pair(net::HTTP_TOO_MANY_REQUESTS, MCPErrorCode::kHttpError),
        std::pair(net::HTTP_INTERNAL_SERVER_ERROR, MCPErrorCode::kHttpError),
        std::pair(net::HTTP_SERVICE_UNAVAILABLE, MCPErrorCode::kHttpError)}) {
    SCOPED_TRACE(status);
    base::test::TestFuture<ToolsResult> future;
    client_->GetTools(future.GetCallback());
    ASSERT_EQ(Method(index), "initialize");
    Respond(index++, status, "text/plain", "");
    ToolsResult result = Take(future);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, code);
  }
}

TEST_F(MCPClientTest, EndSessionStartsAnotherWithInitialize) {
  base::test::TestFuture<ToolsResult> listed;
  client_->GetTools(listed.GetCallback());
  const size_t list = CompleteHandshake(0);
  RespondResult(list, R"({"tools":[]})");
  ASSERT_TRUE(Take(listed).has_value());

  client_->EndSession();
  RunReadyTasks();
  // The session is ended, and its tools aren't served from the cache.
  ASSERT_EQ(RequestCount(), list + 2);
  EXPECT_EQ(Request(list + 1).method, "DELETE");
  EXPECT_EQ(Header(list + 1, "Mcp-Session-Id"), kSessionId);
  base::test::TestFuture<ToolsResult> relisted;
  client_->GetTools(relisted.GetCallback());
  EXPECT_EQ(Method(list + 2), "initialize");
  EXPECT_FALSE(Request(list + 2).headers.HasHeader("Mcp-Session-Id"));
}

TEST_F(MCPClientTest, EndSessionLeavesCallsInFlight) {
  base::test::TestFuture<ToolsResult> listed;
  client_->GetTools(listed.GetCallback());
  const size_t list = CompleteHandshake(0);
  RespondResult(list, R"({"tools":[]})");
  ASSERT_TRUE(Take(listed).has_value());
  base::test::TestFuture<base::expected<MCPCallToolResult, MCPError>> called;
  base::OnceClosure cancel =
      client_->CallTool("search", base::DictValue(), called.GetCallback());
  const size_t call = list + 1;
  ASSERT_EQ(Method(call), "tools/call");

  // Not ended on the server while it's working on the call.
  client_->EndSession();
  RunReadyTasks();
  EXPECT_EQ(RequestCount(), call + 1);
  // The next request starts a session of its own meanwhile.
  base::test::TestFuture<ToolsResult> relisted;
  client_->GetTools(relisted.GetCallback());
  EXPECT_EQ(Method(call + 1), "initialize");

  // Ended once the call has finished.
  RespondResult(call, R"({"content":[{"type":"text","text":"Done"}]})");
  EXPECT_TRUE(Take(called).has_value());
  ASSERT_EQ(RequestCount(), call + 3);
  EXPECT_EQ(Request(call + 2).method, "DELETE");
  EXPECT_EQ(Header(call + 2, "Mcp-Session-Id"), kSessionId);
}

TEST_F(MCPClientTest, ErrorWithNullIdOnStream) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("echo", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  const std::string long_tail(1000, 'x');
  Respond(call, net::HTTP_OK, "text/event-stream",
          MakeSSEEvent(base::StrCat(
              {R"({"jsonrpc":"2.0","id":null,"error":{"code":-32000,)",
               R"("message":"Line 1\n## Line 2)", long_tail, R"("}})"})));

  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kServerError);
  // Control characters are replaced and the text is bounded.
  EXPECT_THAT(result.error().message, HasSubstr("Line 1?## Line 2xxx"));
  EXPECT_LT(result.error().message.size(), 600u);
}

TEST_F(MCPClientTest, ServerTextFormatCharactersAreReplaced) {
  base::test::TestFuture<CallResult> future;
  base::OnceClosure cancel =
      client_->CallTool("echo", base::DictValue(), future.GetCallback());
  const size_t call = CompleteHandshake(0);
  // Bidi overrides and isolates, a zero-width space, a line separator, a C1
  // control and a byte order mark, around text that's kept.
  Respond(
      call, net::HTTP_OK, "application/json",
      base::StrCat({R"({"jsonrpc":"2.0","id":)", base::NumberToString(Id(call)),
                    R"(,"error":{"code":1,"message":)",
                    R"("a\u202eb\u2066c\u2069d\u200be\u2028f\u0085g\ufeffh )",
                    R"(\u00e9\u4e2d"}})"}));

  CallResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_THAT(result.error().message,
              HasSubstr("a?b?c?d?e?f?g?h \xC3\xA9\xE4\xB8\xAD"));
}

TEST_F(MCPClientTest, MalformedResponsesAreInvalid) {
  for (const char* response_template : {
           R"({"jsonrpc":"2.0","id":999,"result":{}})",
           R"({"jsonrpc":"2.0","id":"ID","result":{}})",
           R"({"jsonrpc":"2.0","id":ID,"result":"ok"})",
           R"({"jsonrpc":"2.0","id":ID,"result":{},"error":{}})",
       }) {
    SCOPED_TRACE(response_template);
    CreateClient();
    const size_t start = RequestCount();
    base::test::TestFuture<CallResult> future;
    base::OnceClosure cancel =
        client_->CallTool("echo", base::DictValue(), future.GetCallback());
    const size_t call = CompleteHandshake(start);
    std::string response = response_template;
    base::ReplaceFirstSubstringAfterOffset(&response, 0, "ID",
                                           base::NumberToString(Id(call)));
    Respond(call, net::HTTP_OK, "text/event-stream", MakeSSEEvent(response));

    CallResult result = Take(future);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, MCPErrorCode::kInvalidResponse);
  }
}

TEST_F(MCPClientTest, CrossOriginRedirect) {
  base::test::TestFuture<ToolsResult> future;
  client_->GetTools(future.GetCallback());
  Redirect(0, GURL("https://evil.example.com/collect?token=1"));

  ToolsResult result = Take(future);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, MCPErrorCode::kCrossOriginRedirect);
  EXPECT_THAT(result.error().message, HasSubstr("https://evil.example.com"));
  EXPECT_THAT(result.error().message, testing::Not(HasSubstr("token")));
  EXPECT_EQ(client_->state(), MCPClient::State::kFailed);
}

}  // namespace ai_chat
