/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_test_server.h"

#include <optional>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"

namespace ai_chat {

namespace {

constexpr char kInitializeResult[] = R"({"protocolVersion":"2025-11-25",
    "capabilities":{"tools":{}},
    "serverInfo":{"name":"test","version":"1"}})";

}  // namespace

MCPTestServer::MCPTestServer(network::TestURLLoaderFactory& url_loader_factory)
    : url_loader_factory_(url_loader_factory) {
  url_loader_factory_->SetInterceptor(
      base::BindRepeating(&MCPTestServer::OnRequest, base::Unretained(this)));
}

MCPTestServer::~MCPTestServer() {
  url_loader_factory_->SetInterceptor({});
}

void MCPTestServer::WaitForRequest(size_t index) {
  // Quit as soon as the request is sent, rather than once the thread is idle:
  // under MOCK_TIME, an idle thread fast-forwards the clock, which would time
  // the requests out.
  while (index >= RequestCount()) {
    base::RunLoop run_loop;
    quit_ = run_loop.QuitClosure();
    run_loop.Run();
  }
  quit_.Reset();
}

size_t MCPTestServer::RequestCount() const {
  return url_loader_factory_->pending_requests()->size();
}

const network::ResourceRequest& MCPTestServer::Request(size_t index) const {
  CHECK_LT(index, RequestCount());
  return url_loader_factory_->pending_requests()->at(index).request;
}

base::DictValue MCPTestServer::Body(size_t index) const {
  std::optional<base::DictValue> body = base::JSONReader::ReadDict(
      network::GetUploadData(Request(index)), base::JSON_PARSE_RFC);
  return body ? std::move(*body) : base::DictValue();
}

std::string MCPTestServer::Method(size_t index) const {
  const base::DictValue body = Body(index);
  const std::string* method = body.FindString("method");
  return method ? *method : std::string();
}

void MCPTestServer::RespondResult(size_t index, std::string_view result_json) {
  Respond(index, 200, ResultMessage(index, result_json));
}

void MCPTestServer::RespondError(size_t index,
                                 int code,
                                 std::string_view message) {
  const std::optional<int> id = Body(index).FindInt("id");
  CHECK(id);
  base::DictValue response;
  response.Set("jsonrpc", "2.0");
  response.Set("id", *id);
  response.Set("error",
               base::DictValue().Set("code", code).Set("message", message));
  Respond(index, 200, *base::WriteJson(response));
}

void MCPTestServer::RespondAccepted(size_t index) {
  Respond(index, 202, "");
}

size_t MCPTestServer::CompleteHandshake(
    size_t index,
    std::optional<std::string_view> session_id) {
  WaitForRequest(index);
  CHECK_EQ(Method(index), "initialize");
  Respond(index, 200, ResultMessage(index, kInitializeResult), session_id);
  WaitForRequest(index + 1);
  CHECK_EQ(Method(index + 1), "notifications/initialized");
  RespondAccepted(index + 1);
  return index + 2;
}

std::string MCPTestServer::ResultMessage(size_t index,
                                         std::string_view result_json) const {
  const std::optional<int> id = Body(index).FindInt("id");
  CHECK(id);
  return base::StrCat({R"({"jsonrpc":"2.0","id":)", base::NumberToString(*id),
                       R"(,"result":)", result_json, "}"});
}

void MCPTestServer::Respond(size_t index,
                            int status,
                            std::string_view body,
                            std::optional<std::string_view> session_id) {
  CHECK_LT(index, RequestCount());
  auto head = network::mojom::URLResponseHead::New();
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head->headers->ReplaceStatusLine(
      base::StrCat({"HTTP/1.1 ", base::NumberToString(status), " Status"}));
  if (session_id) {
    head->headers->SetHeader("Mcp-Session-Id", *session_id);
  }
  if (!body.empty()) {
    head->headers->SetHeader("Content-Type", "application/json");
    head->mime_type = "application/json";
  }
  url_loader_factory_->SimulateResponseWithoutRemovingFromPendingList(
      &url_loader_factory_->pending_requests()->at(index), std::move(head),
      body, network::URLLoaderCompletionStatus(net::OK));
}

void MCPTestServer::OnRequest(const network::ResourceRequest& request) {
  // It's added to the pending requests after this, and before the quit takes
  // effect.
  if (quit_) {
    quit_.Run();
  }
}

}  // namespace ai_chat
