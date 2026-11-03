/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/sse_parser.h"

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace api_request_helper {

class SSEParserTest : public testing::Test {
 public:
  void SetUp() override {
    parser_ = std::make_unique<SSEParser>(
        base::SequencedTaskRunner::GetCurrentDefault(),
        base::BindRepeating(&SSEParserTest::OnEvent, base::Unretained(this)));
  }

 protected:
  void OnEvent(ValueOrError result) { results_.push_back(std::move(result)); }

  // Feeds `chunk` and waits for any events it dispatches to be parsed.
  void Process(std::string_view chunk) {
    parser_->Process(chunk);
    WaitForDecoding();
  }

  void Flush() {
    parser_->Flush();
    WaitForDecoding();
  }

  // Waits for the events dispatched so far to be delivered.
  void WaitForDecoding() {
    base::test::TestFuture<void> events_delivered;
    parser_->RunAfterPendingEvents(events_delivered.GetCallback());
    EXPECT_TRUE(events_delivered.Wait());
  }

  // Returns the integer value of `key` in the `index`th result.
  std::optional<int> FindInt(size_t index, std::string_view key) {
    if (index >= results_.size() || !results_[index].has_value() ||
        !results_[index]->is_dict()) {
      return std::nullopt;
    }
    return results_[index]->GetDict().FindInt(key);
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<SSEParser> parser_;
  std::vector<ValueOrError> results_;
};

TEST_F(SSEParserTest, ParsesSingleEvent) {
  Process("data: {\"key\": \"val\"}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  ASSERT_TRUE(results_[0].has_value());
  EXPECT_EQ(*results_[0]->GetDict().FindString("key"), "val");
}

TEST_F(SSEParserTest, DataWithAndWithoutSpace) {
  Process("data:{\"a\":1}\n\ndata: {\"b\":2}\n\ndata:  {\"c\":3}\n\n");
  ASSERT_EQ(results_.size(), 3u);
  EXPECT_EQ(FindInt(0, "a"), 1);
  EXPECT_EQ(FindInt(1, "b"), 2);
  EXPECT_EQ(FindInt(2, "c"), 3);
}

TEST_F(SSEParserTest, DispatchesOnBlankLine) {
  parser_->Process("data: {\"a\":1}\n");
  WaitForDecoding();
  EXPECT_TRUE(results_.empty());
  Process("\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "a"), 1);
}

TEST_F(SSEParserTest, JoinsMultiLineData) {
  Process("data: {\"a\":1,\ndata:\"b\":\ndata: 2}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "a"), 1);
  EXPECT_EQ(FindInt(0, "b"), 2);
}

TEST_F(SSEParserTest, MultipleEventsInOneChunk) {
  Process("data: {\"a\":1}\n\ndata: {\"b\":2}\n\n");
  ASSERT_EQ(results_.size(), 2u);
  EXPECT_EQ(FindInt(0, "a"), 1);
  EXPECT_EQ(FindInt(1, "b"), 2);
}

TEST_F(SSEParserTest, EventsSeparatedBySingleNewlineAreJoined) {
  // Without a blank line between them, both `data:` lines belong to one event,
  // whose joined data is not valid JSON.
  Process("data: {\"a\":1}\ndata: {\"b\":2}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_FALSE(results_[0].has_value());
}

TEST_F(SSEParserTest, LineSplitAcrossChunks) {
  parser_->Process("data: {\"k\":");
  WaitForDecoding();
  EXPECT_TRUE(results_.empty());
  Process("\"v\"}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  ASSERT_TRUE(results_[0].has_value());
  EXPECT_EQ(*results_[0]->GetDict().FindString("k"), "v");
}

TEST_F(SSEParserTest, EventSplitAcrossChunks) {
  parser_->Process("data: {\"a\":");
  parser_->Process("1,\ndata: \"b\"");
  parser_->Process(":2}\n");
  WaitForDecoding();
  EXPECT_TRUE(results_.empty());
  Process("\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "a"), 1);
  EXPECT_EQ(FindInt(0, "b"), 2);
}

TEST_F(SSEParserTest, LineEndings) {
  Process(
      "data: {\"lf\":1}\n\n"
      "data: {\"cr\":2}\r\r"
      "data: {\"crlf\":3}\r\n\r\n");
  ASSERT_EQ(results_.size(), 3u);
  EXPECT_EQ(FindInt(0, "lf"), 1);
  EXPECT_EQ(FindInt(1, "cr"), 2);
  EXPECT_EQ(FindInt(2, "crlf"), 3);
}

TEST_F(SSEParserTest, CRLFIsOneLineEnding) {
  Process("data: {\"a\":1,\r\ndata: \"b\":2}\r\n\r\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "a"), 1);
  EXPECT_EQ(FindInt(0, "b"), 2);
}

TEST_F(SSEParserTest, CRLFSplitAcrossChunks) {
  parser_->Process("data: {\"a\":1,\r");
  parser_->Process("\ndata: \"b\":2}\r");
  parser_->Process("\n");
  WaitForDecoding();
  EXPECT_TRUE(results_.empty());
  Process("\r\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "a"), 1);
  EXPECT_EQ(FindInt(0, "b"), 2);
}

TEST_F(SSEParserTest, IgnoresCommentsAndOtherFields) {
  Process(
      ": keepalive\n\n"
      "event: message\n"
      "id: 1\n"
      "retry: 1000\n"
      ":comment\n"
      "data: {\"x\":1}\n"
      "unknown: {\"y\":2}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  ASSERT_TRUE(results_[0].has_value());
  EXPECT_EQ(*results_[0], base::test::ParseJson("{\"x\":1}"));
}

TEST_F(SSEParserTest, EventWithEmptyDataIsNotDispatched) {
  // An event with only an id and empty data.
  Process("id: 0\ndata:\n\n");
  Process("id: 1\ndata\n\n");
  Process("id: 2\n\n");
  Process("retry: 1000\n\n");
  EXPECT_TRUE(results_.empty());

  Process("id: 3\ndata: {\"x\":1}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "x"), 1);
}

TEST_F(SSEParserTest, NonObjectDataIsNotDispatched) {
  Process(
      "data: [DONE]\n\n"
      "data: [1, 2]\n\n"
      "data: \"text\"\n\n"
      "data: 42\n\n"
      "data: plain text\n\n");
  EXPECT_TRUE(results_.empty());
}

TEST_F(SSEParserTest, SkipsLeadingBOM) {
  Process(
      "\xEF\xBB\xBF"
      "data: {\"a\":1}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "a"), 1);

  // Only at the start of the stream: here it becomes part of the field name.
  Process(
      "\xEF\xBB\xBF"
      "data: {\"b\":2}\n\n");
  EXPECT_EQ(results_.size(), 1u);

  // A new stream may start with a BOM split across chunks.
  parser_->Clear();
  parser_->Process("\xEF");
  parser_->Process("\xBB");
  Process(
      "\xBF"
      "data: {\"c\":3}\n\n");
  ASSERT_EQ(results_.size(), 2u);
  EXPECT_EQ(FindInt(1, "c"), 3);
}

TEST_F(SSEParserTest, InvalidJsonProducesError) {
  Process("data: {not valid json}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_FALSE(results_[0].has_value());
}

TEST_F(SSEParserTest, FlushDispatchesEventWithoutBlankLine) {
  Process("data: {\"a\":1}\n\ndata: {\"b\":2}\n");
  ASSERT_EQ(results_.size(), 1u);
  Flush();
  ASSERT_EQ(results_.size(), 2u);
  EXPECT_EQ(FindInt(1, "b"), 2);
}

TEST_F(SSEParserTest, FlushDiscardsUnterminatedLine) {
  parser_->Process("data: {\"a\":1}");
  Flush();
  EXPECT_TRUE(results_.empty());

  Process("data: {\"b\":2}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "b"), 2);
}

TEST_F(SSEParserTest, ClearDropsBufferedInput) {
  parser_->Process("data: {\"a\":1}\ndata: {\"k\":");
  parser_->Clear();
  Flush();
  EXPECT_TRUE(results_.empty());

  Process("data: {\"x\":1}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_EQ(FindInt(0, "x"), 1);
}

TEST_F(SSEParserTest, RunAfterPendingEventsFollowsEvents) {
  parser_->Process("data: {\"a\":1}\n\ndata: {\"b\":2}\n\ndata: {\"c\":3}\n\n");
  base::test::TestFuture<size_t> events_before_callback;
  parser_->RunAfterPendingEvents(base::BindLambdaForTesting(
      [&]() { events_before_callback.SetValue(results_.size()); }));
  EXPECT_EQ(events_before_callback.Get(), 3u);
}

TEST_F(SSEParserTest, ClearDropsPendingEvents) {
  parser_->Process("data: {\"a\":1}\n\n");
  parser_->RunAfterPendingEvents(base::BindOnce([]() { ADD_FAILURE(); }));
  parser_->Clear();
  Process("data: {\"b\":2}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  ASSERT_TRUE(results_[0].has_value());
  EXPECT_EQ(results_[0]->GetDict().FindInt("b"), 2);
}

}  // namespace api_request_helper
