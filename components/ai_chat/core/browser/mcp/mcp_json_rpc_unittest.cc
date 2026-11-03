/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_json_rpc.h"

#include <optional>
#include <variant>

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJsonDict;

namespace ai_chat {

TEST(MCPJsonRpcTest, MakeJsonRpcMessages) {
  EXPECT_EQ(MakeJsonRpcRequest(3, "tools/list", std::nullopt),
            ParseJsonDict(R"({"jsonrpc":"2.0","id":3,"method":"tools/list"})"));
  EXPECT_EQ(
      MakeJsonRpcRequest(4, "tools/call", ParseJsonDict(R"({"name":"a"})")),
      ParseJsonDict(
          R"({"jsonrpc":"2.0","id":4,"method":"tools/call",
              "params":{"name":"a"}})"));
  EXPECT_EQ(MakeJsonRpcNotification("notifications/initialized", std::nullopt),
            ParseJsonDict(
                R"({"jsonrpc":"2.0","method":"notifications/initialized"})"));
  EXPECT_EQ(MakeJsonRpcResult(JsonRpcId("abc"), base::DictValue()),
            ParseJsonDict(R"({"jsonrpc":"2.0","id":"abc","result":{}})"));
  EXPECT_EQ(MakeJsonRpcErrorResponse(JsonRpcId(7), -32601, "Method not found"),
            ParseJsonDict(R"({"jsonrpc":"2.0","id":7,
                "error":{"code":-32601,"message":"Method not found"}})"));
}

TEST(MCPJsonRpcTest, ParseJsonRpcRequest) {
  for (const char* json : {R"({"jsonrpc":"2.0","id":5,"method":"ping"})",
                           R"({"jsonrpc":"2.0","id":"x","method":"ping",
                               "params":{},"unknown":true})"}) {
    SCOPED_TRACE(json);
    auto message = ParseJsonRpcMessage(ParseJsonDict(json));
    ASSERT_TRUE(message);
    auto* request = std::get_if<JsonRpcRequest>(&*message);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->method, "ping");
  }
  auto message = ParseJsonRpcMessage(
      ParseJsonDict(R"({"jsonrpc":"2.0","id":"x","method":"ping"})"));
  ASSERT_TRUE(message);
  EXPECT_EQ(std::get<JsonRpcRequest>(*message).id, JsonRpcId("x"));
}

TEST(MCPJsonRpcTest, ParseJsonRpcNotification) {
  auto message = ParseJsonRpcMessage(ParseJsonDict(
      R"({"jsonrpc":"2.0","method":"notifications/progress",
          "params":{"progressToken":2,"progress":1}})"));
  ASSERT_TRUE(message);
  auto* notification = std::get_if<JsonRpcNotification>(&*message);
  ASSERT_TRUE(notification);
  EXPECT_EQ(notification->method, "notifications/progress");
  ASSERT_TRUE(notification->params);
  EXPECT_EQ(notification->params->FindInt("progressToken"), 2);

  message = ParseJsonRpcMessage(
      ParseJsonDict(R"({"jsonrpc":"2.0","method":"notifications/message"})"));
  ASSERT_TRUE(message);
  EXPECT_FALSE(std::get<JsonRpcNotification>(*message).params);
}

TEST(MCPJsonRpcTest, ParseJsonRpcResponse) {
  auto message = ParseJsonRpcMessage(
      ParseJsonDict(R"({"jsonrpc":"2.0","id":1,"result":{"a":1}})"));
  ASSERT_TRUE(message);
  auto* response = std::get_if<JsonRpcResponse>(&*message);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->id, JsonRpcId(1));
  ASSERT_TRUE(response->result.has_value());
  EXPECT_EQ(*response->result, ParseJsonDict(R"({"a":1})"));

  message = ParseJsonRpcMessage(ParseJsonDict(
      R"({"jsonrpc":"2.0","id":2,"error":{"code":-1,"message":"bad"}})"));
  ASSERT_TRUE(message);
  response = std::get_if<JsonRpcResponse>(&*message);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->id, JsonRpcId(2));
  EXPECT_EQ(response->result.error(), (JsonRpcError{-1, "bad"}));

  // An error the server couldn't attribute to a request.
  for (const char* json :
       {R"({"jsonrpc":"2.0","id":null,"error":{"code":-1,"message":"m"}})",
        R"({"jsonrpc":"2.0","error":{"code":-1,"message":"m"}})"}) {
    SCOPED_TRACE(json);
    message = ParseJsonRpcMessage(ParseJsonDict(json));
    ASSERT_TRUE(message);
    response = std::get_if<JsonRpcResponse>(&*message);
    ASSERT_TRUE(response);
    EXPECT_FALSE(response->id);
    EXPECT_FALSE(response->result.has_value());
  }
}

TEST(MCPJsonRpcTest, ParseJsonRpcMessageRejectsMalformed) {
  for (const char* json : {
           R"({"id":1,"result":{}})",
           R"({"jsonrpc":"1.0","id":1,"result":{}})",
           R"({"jsonrpc":2,"id":1,"result":{}})",
           R"({"jsonrpc":"2.0","id":1})",
           R"({"jsonrpc":"2.0","id":1,"result":{},"error":{}})",
           R"({"jsonrpc":"2.0","id":1,"result":[]})",
           R"({"jsonrpc":"2.0","id":1,"result":"ok"})",
           R"({"jsonrpc":"2.0","result":{}})",
           R"({"jsonrpc":"2.0","id":null,"result":{}})",
           R"({"jsonrpc":"2.0","id":1.5,"result":{}})",
           R"({"jsonrpc":"2.0","id":true,"result":{}})",
           R"({"jsonrpc":"2.0","id":{},"result":{}})",
           R"({"jsonrpc":"2.0","id":1,"error":"bad"})",
           R"({"jsonrpc":"2.0","id":1,"error":{"message":"m"}})",
           R"({"jsonrpc":"2.0","id":1,"error":{"code":"1","message":"m"}})",
           R"({"jsonrpc":"2.0","id":1,"error":{"code":1}})",
           R"({"jsonrpc":"2.0","id":1,"method":5})",
           R"({"jsonrpc":"2.0","id":null,"method":"ping"})",
           R"({"jsonrpc":"2.0","id":[],"method":"ping"})",
           R"({"jsonrpc":"2.0","method":"n","params":[]})",
       }) {
    SCOPED_TRACE(json);
    EXPECT_FALSE(ParseJsonRpcMessage(ParseJsonDict(json)));
  }
}

}  // namespace ai_chat
