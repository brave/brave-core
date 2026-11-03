/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HTTP_TEST_BASE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HTTP_TEST_BASE_H_

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class GURL;

namespace network {
struct ResourceRequest;
}  // namespace network

namespace ai_chat {

using MCPTestHeaders = base::flat_map<std::string, std::string>;

network::mojom::URLResponseHeadPtr MakeMCPResponseHead(
    int status,
    std::string_view mime_type,
    const MCPTestHeaders& headers = {});

// An SSE event with one `data:` line per line of `data`.
std::string MakeSSEEvent(std::string_view data);

// Plays an MCP server's HTTP side through a TestURLLoaderFactory, on a mock
// clock so that timeouts only fire when a test advances it. Requests are
// indexed in the order they were sent, including those already answered.
class MCPHttpTestBase : public testing::Test {
 protected:
  MCPHttpTestBase();
  ~MCPHttpTestBase() override;

  // Lets detached requests, which own themselves, time out and be freed.
  // Subclasses destroy what they're testing first.
  void TearDown() override;

  // Runs whatever is ready, on the main thread and the thread pool, without
  // advancing the mock clock.
  void RunReadyTasks();

  template <typename T>
  T Take(base::test::TestFuture<T>& future) {
    RunReadyTasks();
    EXPECT_TRUE(future.IsReady());
    return future.Take();
  }

  size_t RequestCount();
  // Runs ready tasks first, so the request may have just been sent.
  network::TestURLLoaderFactory::PendingRequest& Pending(size_t index);
  const network::ResourceRequest& Request(size_t index);
  base::DictValue Body(size_t index);
  std::optional<std::string> Header(size_t index, std::string_view name);

  void Respond(size_t index,
               int status,
               std::string_view mime_type,
               std::string_view body,
               const MCPTestHeaders& headers = {});
  // Answers a notification or a reply.
  void RespondAccepted(size_t index);
  // Answers with a 307 to `target`.
  void Redirect(size_t index, const GURL& target);
  // Starts a streamed SSE reply, whose body is written to the returned
  // producer.
  mojo::ScopedDataPipeProducerHandle StartStream(
      size_t index,
      const MCPTestHeaders& headers = {});

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory url_loader_factory_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HTTP_TEST_BASE_H_
