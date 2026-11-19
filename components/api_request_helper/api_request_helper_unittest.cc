/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/byte_size.h"
#include "base/containers/span.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace api_request_helper {

namespace {

// Waits for JSON parses already posted to the thread pool, and their replies.
// Needed to assert that an event is not delivered, which no `RunUntil()`
// condition can wait for.
void WaitForPendingParses() {
  base::ThreadPoolInstance::Get()->FlushForTesting();
  base::test::TestFuture<void> replies_done;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, replies_done.GetCallback());
  EXPECT_TRUE(replies_done.Wait());
}

std::optional<std::string> ConversionCallback(
    const std::string& expected_raw_response,
    const std::optional<std::string>& converted_response,
    const std::string& raw_response) {
  EXPECT_EQ(expected_raw_response, raw_response);
  return converted_response;
}
}  // namespace

class ApiRequestHelperUnitTest : public testing::Test {
 public:
  ApiRequestHelperUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    api_request_helper_ = std::make_unique<APIRequestHelper>(
        net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        shared_url_loader_factory_);
    loader_wrapper_handler_ =
        std::make_unique<APIRequestHelper::URLLoaderHandler>(
            api_request_helper_.get(),
            base::SequencedTaskRunner::GetCurrentDefault());
  }
  ~ApiRequestHelperUnitTest() override = default;

  void SetInterceptor(const std::string& expected_method,
                      const GURL& expected_url,
                      const std::string& content_to_respond,
                      bool enable_cache,
                      net::HttpStatusCode status = net::HTTP_OK,
                      std::optional<std::string> content_type = std::nullopt) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_method, expected_url, content_to_respond, enable_cache,
         status, content_type](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          EXPECT_EQ(request.url, expected_url);
          EXPECT_EQ(request.method, expected_method);
          if (enable_cache) {
            EXPECT_EQ(request.load_flags, net::LOAD_DO_NOT_SAVE_COOKIES);
          } else {
            EXPECT_EQ(request.load_flags, net::LOAD_DO_NOT_SAVE_COOKIES |
                                              net::LOAD_BYPASS_CACHE |
                                              net::LOAD_DISABLE_CACHE);
          }
          if (content_type) {
            auto head = network::mojom::URLResponseHead::New();
            head->headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
            head->headers->ReplaceStatusLine(
                "HTTP/1.1 " + base::NumberToString(static_cast<int>(status)) +
                " " + std::string(net::GetHttpReasonPhrase(status)));
            head->headers->SetHeader("Content-Type", *content_type);
            head->mime_type = *content_type;
            url_loader_factory_.AddResponse(
                request.url, std::move(head), content_to_respond,
                network::URLLoaderCompletionStatus(net::OK));
          } else {
            url_loader_factory_.AddResponse(request.url.spec(),
                                            content_to_respond, status);
          }
        }));
  }

  void SendRequest(const std::string& server_raw_response,
                   const base::Value& expected_value_body,
                   const int expected_http_code = 200,
                   const int expected_error_code = net::OK,
                   APIRequestHelper::ResponseConversionCallback
                       conversion_callback = base::NullCallback(),
                   bool enable_cache = false) {
    GURL network_url("http://localhost/");

    APIRequestResult expected_result(
        expected_http_code, expected_value_body.Clone(),
        {{"content-type", "text/html"}}, expected_error_code, network_url);
    base::test::TestFuture<APIRequestResult> future;

    SetInterceptor("POST", network_url, server_raw_response, enable_cache);
    api_request_helper_->Request(
        "POST", network_url, "", "application/json", future.GetCallback(), {},
        APIRequestOptions(false, enable_cache, -1u, std::nullopt),
        std::move(conversion_callback));
    EXPECT_EQ(future.Take(), expected_result);
  }

  void SendMessageSSEJSON(std::string_view string_piece,
                          APIRequestHelper::DataReceivedCallback callback) {
    loader_wrapper_handler_->send_sse_data_for_testing(string_piece, true,
                                                       std::move(callback));
  }

  void SendMessageSSE(std::string_view string_piece,
                      APIRequestHelper::DataReceivedCallback callback) {
    loader_wrapper_handler_->send_sse_data_for_testing(string_piece, false,
                                                       std::move(callback));
  }

  // Answers the latest request with an event stream `body`, then completes it
  // with `net_error`. Returns once the loader has read and completed it.
  void RespondWithEventStream(std::string_view body, int net_error = net::OK) {
    ASSERT_FALSE(url_loader_factory_.pending_requests()->empty());
    const size_t index = url_loader_factory_.pending_requests()->size() - 1;
    auto& client = url_loader_factory_.pending_requests()->at(index).client;

    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    ASSERT_EQ(mojo::CreateDataPipe(body.size(), producer, consumer),
              MOJO_RESULT_OK);
    ASSERT_EQ(producer->WriteAllData(base::as_byte_span(body)), MOJO_RESULT_OK);
    producer.reset();

    auto head = network::CreateURLResponseHead(net::HTTP_OK);
    head->headers->SetHeader("Content-Type", "text/event-stream");
    head->mime_type = "text/event-stream";
    client->OnReceiveResponse(std::move(head), std::move(consumer),
                              std::nullopt);
    network::URLLoaderCompletionStatus status(net_error);
    status.decoded_body_length = base::ByteSize(body.size());
    client->OnComplete(status);

    // The loader closes the client pipe when it has the completion. As
    // `RunUntil()` checks only once the thread is idle, the loader has also
    // read the body by then. A retry may add a pending request, so `client` is
    // not used again.
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return !url_loader_factory_.pending_requests()
                  ->at(index)
                  .client.is_connected();
    }));
  }

 protected:
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;

 private:
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<APIRequestHelper::URLLoaderHandler> loader_wrapper_handler_;
};

TEST_F(ApiRequestHelperUnitTest, SanitizedRequest) {
  std::string expected_sanitized_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":1.8446744073709552e+19}";
  std::string server_raw_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":18446744073709551615}";
  SendRequest(server_raw_response, ParseJson(expected_sanitized_response));
  SendRequest("", base::Value());
  SendRequest("{}", base::Value(base::Value::Type::DICT));
  SendRequest("{", base::Value());
  SendRequest("0", base::Value());
  SendRequest("a", base::Value());
  // The sanitizer doesn't support trailing commas by default.
  SendRequest("{\"a\":1,}", base::Value());
}

TEST_F(ApiRequestHelperUnitTest, RequestWithConversion) {
  std::string expected_sanitized_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"18446744073709551615\"}";
  std::string server_raw_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":18446744073709551615}";
  SendRequest(server_raw_response, ParseJson(expected_sanitized_response), 200,
              net::OK,
              base::BindOnce(&ConversionCallback, server_raw_response,
                             expected_sanitized_response));

  // Broken json after conversion
  // expecting empty response body after sanitization
  SendRequest(
      server_raw_response, base::Value(), 200, net::OK,
      base::BindOnce(&ConversionCallback, server_raw_response, "broken json"));
  // Empty json after conversion
  // expecting empty response body after sanitization
  SendRequest(server_raw_response, base::Value(), 200, net::OK,
              base::BindOnce(&ConversionCallback, server_raw_response, ""));

  // Returning std::nullopt in conversion callback results in empty response
  server_raw_response = "{}";
  SendRequest(
      server_raw_response, base::Value(), 422, net::OK,
      base::BindOnce(&ConversionCallback, server_raw_response, std::nullopt));
}

TEST_F(ApiRequestHelperUnitTest, Is2XXResponseCode) {
  EXPECT_TRUE(
      APIRequestResult(200, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_TRUE(
      APIRequestResult(201, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_TRUE(
      APIRequestResult(250, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_TRUE(
      APIRequestResult(299, {}, {}, net::OK, GURL()).Is2XXResponseCode());

  EXPECT_FALSE(
      APIRequestResult(0, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(1, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(-1, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(199, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(300, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(500, {}, {}, net::OK, GURL()).Is2XXResponseCode());
}

TEST_F(ApiRequestHelperUnitTest, EnableCache) {
  SendRequest("{}", base::Value(base::Value::Type::DICT), 200, net::OK,
              base::NullCallback(), false);
  SendRequest("{}", base::Value(base::Value::Type::DICT), 200, net::OK,
              base::NullCallback(), true);
}

TEST_F(ApiRequestHelperUnitTest, URLLoaderHandlerParsing) {
  base::test::TestFuture<ValueOrError> future;
  SendMessageSSE("data: [DONE]", future.GetRepeatingCallback());
  ValueOrError result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->is_string());
  EXPECT_EQ("data: [DONE]", result->GetString());
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsing) {
  base::test::TestFuture<ValueOrError> future;
  SendMessageSSEJSON(
      "data: {\"completion\": \" Hello there!\", \"stop\": null}\n\n",
      future.GetRepeatingCallback());
  {
    ValueOrError result = future.Take();
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->is_dict());
    const std::string* completion = result->GetDict().FindString("completion");
    ASSERT_TRUE(completion);
    EXPECT_EQ(" Hello there!", *completion);
  }

  SendMessageSSEJSON(
      "data: {\"completion\": \" Hello there! How are you?\", \"stop\": "
      "null}\n\n",
      future.GetRepeatingCallback());
  {
    ValueOrError result = future.Take();
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->is_dict());
    const std::string* completion = result->GetDict().FindString("completion");
    ASSERT_TRUE(completion);
    EXPECT_EQ(" Hello there! How are you?", *completion);
  }

  // This test verifies that the callback is not called when the response is
  // "[DONE]".
  SendMessageSSEJSON(
      "data: [DONE]\n\n",
      base::BindRepeating([](ValueOrError result) { ADD_FAILURE(); }));
  task_environment_.RunUntilIdle();

  // Testing with no JSON and an empty string
  SendMessageSSEJSON(
      "", base::BindRepeating([](ValueOrError result) { ADD_FAILURE(); }));
  task_environment_.RunUntilIdle();
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsingChunkedLines) {
  // Simulate a large SSE event split across three OnDataReceived
  // calls, as can happen when network chunks don't align with SSE
  // event boundaries.

  // First chunk: partial line, callback should not fire.
  SendMessageSSEJSON("data: {\"completi",
                     base::BindRepeating([](ValueOrError) { ADD_FAILURE(); }));

  // Second chunk: still partial, callback should not fire.
  SendMessageSSEJSON("on\": \"Hello wor",
                     base::BindRepeating([](ValueOrError) { ADD_FAILURE(); }));

  // Third chunk: completes the line, callback should fire.
  base::RunLoop run_loop;
  SendMessageSSEJSON("ld!\", \"stop\": null}\n\n",
                     base::BindRepeating(
                         [](base::RunLoop* loop, ValueOrError result) {
                           ASSERT_TRUE(result.has_value());
                           ASSERT_TRUE(result->is_dict());
                           const std::string* completion =
                               result->GetDict().FindString("completion");
                           ASSERT_TRUE(completion);
                           EXPECT_EQ("Hello world!", *completion);
                           loop->Quit();
                         },
                         &run_loop));
  run_loop.Run();
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsingMultipleEventsInOneChunk) {
  std::vector<std::string> completions;
  base::RunLoop run_loop;
  SendMessageSSEJSON(
      "data: {\"completion\": \"first\"}\n\n"
      "data: {\"completion\": \"second\"}\n\n"
      "data: {\"completion\": \"third\"}\n\n",
      base::BindRepeating(
          [](base::RunLoop* loop, std::vector<std::string>* completions,
             ValueOrError result) {
            ASSERT_TRUE(result.has_value());
            ASSERT_TRUE(result->is_dict());
            const std::string* completion =
                result->GetDict().FindString("completion");
            ASSERT_TRUE(completion);
            completions->push_back(*completion);
            if (completions->size() == 3u) {
              loop->Quit();
            }
          },
          &run_loop, &completions));
  run_loop.Run();
  EXPECT_EQ(completions,
            std::vector<std::string>({"first", "second", "third"}));
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsingMixedLineEndings) {
  // Verify that \n, \r, and \r\n are all treated as line
  // endings.
  std::vector<std::string> completions;
  base::RunLoop run_loop;
  SendMessageSSEJSON(
      "data: {\"completion\": \"lf\"}\n\n"
      "data: {\"completion\": \"cr\"}\r\r"
      "data: {\"completion\": \"crlf\"}\r\n\r\n",
      base::BindRepeating(
          [](base::RunLoop* loop, std::vector<std::string>* completions,
             ValueOrError result) {
            ASSERT_TRUE(result.has_value());
            ASSERT_TRUE(result->is_dict());
            const std::string* completion =
                result->GetDict().FindString("completion");
            ASSERT_TRUE(completion);
            completions->push_back(*completion);
            if (completions->size() == 3u) {
              loop->Quit();
            }
          },
          &run_loop, &completions));
  run_loop.Run();
  EXPECT_EQ(completions, std::vector<std::string>({"lf", "cr", "crlf"}));
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsingEscapedLineBreaks) {
  // Verify that escaped line breaks (\\n, \\r\\n) inside JSON
  // string values are preserved and don't interfere with SSE line
  // splitting.
  base::RunLoop run_loop;
  SendMessageSSEJSON("data: {\"completion\": \"line1\\nline2\\r\\nline3\"}\n\n",
                     base::BindRepeating(
                         [](base::RunLoop* loop, ValueOrError result) {
                           ASSERT_TRUE(result.has_value());
                           ASSERT_TRUE(result->is_dict());
                           const std::string* completion =
                               result->GetDict().FindString("completion");
                           ASSERT_TRUE(completion);
                           EXPECT_EQ("line1\nline2\r\nline3", *completion);
                           loop->Quit();
                         },
                         &run_loop));
  run_loop.Run();
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsingEscapedDoubleLineBreaks) {
  // Verify that escaped double line breaks (\\n\\n) inside JSON
  // string values are preserved — these look like SSE event
  // separators but should not be treated as such.
  base::RunLoop run_loop;
  SendMessageSSEJSON("data: {\"completion\": \"para1\\n\\npara2\"}\n\n",
                     base::BindRepeating(
                         [](base::RunLoop* loop, ValueOrError result) {
                           ASSERT_TRUE(result.has_value());
                           ASSERT_TRUE(result->is_dict());
                           const std::string* completion =
                               result->GetDict().FindString("completion");
                           ASSERT_TRUE(completion);
                           EXPECT_EQ("para1\n\npara2", *completion);
                           loop->Quit();
                         },
                         &run_loop));
  run_loop.Run();
}

TEST_F(ApiRequestHelperUnitTest, SSE400WithJsonErrorBody) {
  SetInterceptor("POST", GURL("http://localhost/"),
                 R"({"error":{"type":1234,"message":"bad"}})", false,
                 net::HTTP_BAD_REQUEST, "application/json");

  base::test::TestFuture<APIRequestResult> future;
  api_request_helper_->RequestSSE(
      "POST", GURL("http://localhost/"), "", "application/json",
      base::BindRepeating([](ValueOrError) { ADD_FAILURE(); }),
      future.GetCallback(), {}, {});
  APIRequestResult result = future.Take();
  EXPECT_FALSE(result.Is2XXResponseCode());
  EXPECT_EQ(result.response_code(), 400);
  ASSERT_TRUE(result.value_body().is_dict());
  const auto* error = result.value_body().GetDict().FindDict("error");
  ASSERT_TRUE(error);
  EXPECT_EQ(error->FindInt("type"), std::optional<int>(1234));
}

// Events are parsed on a worker sequence, so a stream can complete while
// several of its events are still being parsed. The result follows them.
TEST_F(ApiRequestHelperUnitTest, SSEResultFollowsPendingEvents) {
  const GURL url("http://localhost/");
  std::vector<std::string> deliveries;
  base::test::TestFuture<void> result_future;
  api_request_helper_->RequestSSE(
      "POST", url, "", "application/json",
      base::BindLambdaForTesting([&](ValueOrError value) {
        ASSERT_TRUE(value.has_value() && value->is_dict());
        const std::string* event = value->GetDict().FindString("event");
        ASSERT_TRUE(event);
        deliveries.push_back(*event);
      }),
      base::BindLambdaForTesting([&](APIRequestResult result) {
        EXPECT_EQ(result.error_code(), net::OK);
        deliveries.push_back("result");
        result_future.SetValue();
      }),
      {}, {});
  {
    // Holds the parses until the stream has completed.
    base::test::TaskEnvironment::ParallelExecutionFence fence;
    RespondWithEventStream(
        "data: {\"event\":\"a\"}\n\n"
        "data: {\"event\":\"b\"}\n\n"
        "data: {\"event\":\"c\"}\n\n");
    EXPECT_TRUE(deliveries.empty());
  }
  EXPECT_TRUE(result_future.Wait());
  EXPECT_EQ(deliveries, std::vector<std::string>({"a", "b", "c", "result"}));
}

// A data callback may destroy the helper while the result waits for the
// remaining events. Running that result on the deleted handler is reported
// only under ASan, or as a crash.
TEST_F(ApiRequestHelperUnitTest, DestroyHelperFromDataCallbackBeforeResult) {
  const GURL url("http://localhost/");
  auto helper = std::make_unique<APIRequestHelper>(
      net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &url_loader_factory_));
  size_t data_count = 0;
  helper->RequestSSE("POST", url, "", "application/json",
                     base::BindLambdaForTesting([&](ValueOrError) {
                       ++data_count;
                       helper.reset();
                     }),
                     base::BindOnce([](APIRequestResult) { ADD_FAILURE(); }),
                     {}, {});
  {
    // Holds the parses until the stream has completed.
    base::test::TaskEnvironment::ParallelExecutionFence fence;
    RespondWithEventStream("data: {\"a\":1}\n\ndata: {\"b\":2}\n\n");
  }
  ASSERT_TRUE(base::test::RunUntil([&]() { return data_count > 0; }));
  // So that a second event or the result would be seen.
  WaitForPendingParses();
  EXPECT_EQ(data_count, 1u);
  EXPECT_FALSE(helper);
}

// Nothing is delivered after a data callback cancels the request. Using the
// deleted handler after the callback is only reported under ASan.
TEST_F(ApiRequestHelperUnitTest, CancelFromDataCallback) {
  const GURL url("http://localhost/");
  for (const char* mime_type : {"text/event-stream", "text/plain"}) {
    SCOPED_TRACE(mime_type);
    SetInterceptor("POST", url, "data: {\"a\":1}\n\ndata: {\"b\":2}\n\n",
                   /*enable_cache=*/false, net::HTTP_OK, mime_type);

    size_t data_count = 0;
    APIRequestHelper::Ticket ticket;
    ticket = api_request_helper_->RequestSSE(
        "POST", url, "", "application/json",
        base::BindLambdaForTesting([&](ValueOrError) {
          ++data_count;
          api_request_helper_->Cancel(ticket);
        }),
        base::BindOnce([](APIRequestResult) { ADD_FAILURE(); }), {}, {});
    ASSERT_TRUE(base::test::RunUntil([&]() { return data_count > 0; }));
    // So that a second delivery would be seen.
    WaitForPendingParses();
    EXPECT_EQ(data_count, 1u);
  }
}

// Events of an attempt abandoned for a retry are not delivered, even when
// they are still being parsed at the retry.
TEST_F(ApiRequestHelperUnitTest, SSERetryDropsAbandonedAttemptEvents) {
  const GURL url("http://localhost/");
  APIRequestOptions options;
  options.auto_retry_on_network_change = true;
  std::vector<base::Value> events;
  base::test::TestFuture<APIRequestResult> result_future;
  api_request_helper_->RequestSSE(
      "POST", url, "", "application/json",
      base::BindLambdaForTesting([&](ValueOrError value) {
        ASSERT_TRUE(value.has_value());
        events.push_back(std::move(*value));
      }),
      result_future.GetCallback(), {}, options);
  {
    // Holds the first attempt's parse until the request is retried.
    base::test::TaskEnvironment::ParallelExecutionFence fence;
    RespondWithEventStream("data: {\"attempt\":1}\n\n",
                           net::ERR_NETWORK_CHANGED);
  }
  ASSERT_EQ(url_loader_factory_.pending_requests()->size(), 2u);
  RespondWithEventStream("data: {\"attempt\":2}\n\n");
  EXPECT_EQ(result_future.Take().error_code(), net::OK);
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0], ParseJson(R"({"attempt":2})"));
}

}  // namespace api_request_helper
