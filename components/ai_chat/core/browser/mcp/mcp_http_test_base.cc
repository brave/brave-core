/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_http_test_base.h"

#include <utility>

#include "base/check_op.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/test/values_test_util.h"
#include "mojo/public/c/system/types.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_utils.h"
#include "url/gurl.h"

namespace ai_chat {

network::mojom::URLResponseHeadPtr MakeMCPResponseHead(
    int status,
    std::string_view mime_type,
    const MCPTestHeaders& headers) {
  auto head = network::mojom::URLResponseHead::New();
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head->headers->ReplaceStatusLine(
      base::StrCat({"HTTP/1.1 ", base::NumberToString(status), " ",
                    net::GetHttpReasonPhrase(status)}));
  if (!mime_type.empty()) {
    head->headers->SetHeader("Content-Type", mime_type);
    head->mime_type = std::string(mime_type);
  }
  for (const auto& [name, value] : headers) {
    head->headers->SetHeader(name, value);
  }
  return head;
}

std::string MakeSSEEvent(std::string_view data) {
  std::string event = "event: message\n";
  for (std::string_view line : base::SplitStringPiece(
           data, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
    base::StrAppend(&event, {"data: ", line, "\n"});
  }
  event.push_back('\n');
  return event;
}

MCPHttpTestBase::MCPHttpTestBase() = default;
MCPHttpTestBase::~MCPHttpTestBase() = default;

void MCPHttpTestBase::TearDown() {
  task_environment_.FastForwardUntilNoTasksRemain();
}

void MCPHttpTestBase::RunReadyTasks() {
  task_environment_.FastForwardBy(base::TimeDelta());
}

size_t MCPHttpTestBase::RequestCount() {
  return url_loader_factory_.pending_requests()->size();
}

network::TestURLLoaderFactory::PendingRequest& MCPHttpTestBase::Pending(
    size_t index) {
  RunReadyTasks();
  EXPECT_LT(index, RequestCount()) << "Request " << index << " wasn't sent";
  CHECK_LT(index, RequestCount());
  return url_loader_factory_.pending_requests()->at(index);
}

const network::ResourceRequest& MCPHttpTestBase::Request(size_t index) {
  return Pending(index).request;
}

base::DictValue MCPHttpTestBase::Body(size_t index) {
  return base::test::ParseJsonDict(network::GetUploadData(Request(index)));
}

std::optional<std::string> MCPHttpTestBase::Header(size_t index,
                                                   std::string_view name) {
  return Request(index).headers.GetHeader(name);
}

void MCPHttpTestBase::Respond(size_t index,
                              int status,
                              std::string_view mime_type,
                              std::string_view body,
                              const MCPTestHeaders& headers) {
  url_loader_factory_.SimulateResponseWithoutRemovingFromPendingList(
      &Pending(index), MakeMCPResponseHead(status, mime_type, headers), body,
      network::URLLoaderCompletionStatus(net::OK));
}

void MCPHttpTestBase::RespondAccepted(size_t index) {
  Respond(index, net::HTTP_ACCEPTED, "", "");
}

void MCPHttpTestBase::Redirect(size_t index, const GURL& target) {
  network::TestURLLoaderFactory::PendingRequest& request = Pending(index);
  net::RedirectInfo redirect_info;
  redirect_info.status_code = net::HTTP_TEMPORARY_REDIRECT;
  redirect_info.new_method = request.request.method;
  redirect_info.new_url = target;
  network::mojom::URLResponseHeadPtr head =
      MakeMCPResponseHead(net::HTTP_TEMPORARY_REDIRECT, "");
  head->headers->SetHeader("Location", target.spec());
  request.client->OnReceiveRedirect(redirect_info, std::move(head));
}

mojo::ScopedDataPipeProducerHandle MCPHttpTestBase::StartStream(
    size_t index,
    const MCPTestHeaders& headers) {
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  CHECK_EQ(mojo::CreateDataPipe(64 * 1024, producer, consumer), MOJO_RESULT_OK);
  Pending(index).client->OnReceiveResponse(
      MakeMCPResponseHead(net::HTTP_OK, "text/event-stream", headers),
      std::move(consumer), std::nullopt);
  return producer;
}

}  // namespace ai_chat
