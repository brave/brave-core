/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/sse_parser.h"

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
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

  // Feeds `chunk` and waits for the events it dispatches.
  void Process(std::string_view chunk) {
    parser_->Process(chunk);
    base::test::TestFuture<void> events_delivered;
    parser_->RunAfterPendingEvents(events_delivered.GetCallback());
    EXPECT_TRUE(events_delivered.Wait());
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

TEST_F(SSEParserTest, IgnoresNonDataLines) {
  Process("data: [DONE]\n: comment\ndata: {\"x\":1}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  ASSERT_TRUE(results_[0].has_value());
  EXPECT_EQ(*results_[0]->GetDict().FindInt("x"), 1);
}

TEST_F(SSEParserTest, InvalidJsonProducesError) {
  Process("data: {not valid json}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_FALSE(results_[0].has_value());
}

TEST_F(SSEParserTest, MultipleEventsInOneChunk) {
  Process("data: {\"a\":1}\ndata: {\"b\":2}\n");
  ASSERT_EQ(results_.size(), 2u);
  ASSERT_TRUE(results_[0].has_value());
  ASSERT_TRUE(results_[1].has_value());
  EXPECT_EQ(results_[0]->GetDict().FindInt("a"), 1);
  EXPECT_EQ(results_[1]->GetDict().FindInt("b"), 2);
}

TEST_F(SSEParserTest, LineSplitAcrossChunks) {
  parser_->Process("data: {\"k\":");
  EXPECT_TRUE(results_.empty());
  Process("\"v\"}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  ASSERT_TRUE(results_[0].has_value());
  EXPECT_EQ(*results_[0]->GetDict().FindString("k"), "v");
}

TEST_F(SSEParserTest, ClearDropsPartialLine) {
  parser_->Process("data: {\"k\":");
  parser_->Clear();
  Process("data: {\"x\":1}\n\n");
  ASSERT_EQ(results_.size(), 1u);
  EXPECT_TRUE(results_[0].has_value());
}

TEST_F(SSEParserTest, NonDataLinesAreIgnored) {
  parser_->Process("event: foo\n");
  parser_->Process(": comment\n");
  parser_->Process("id: 123\n");
  EXPECT_TRUE(results_.empty());
}

TEST_F(SSEParserTest, MixedLineEndings) {
  Process("data: {\"a\":1}\r\ndata: {\"b\":2}\r");
  ASSERT_EQ(results_.size(), 2u);
  ASSERT_TRUE(results_[0].has_value());
  ASSERT_TRUE(results_[1].has_value());
  EXPECT_EQ(results_[0]->GetDict().FindInt("a"), 1);
  EXPECT_EQ(results_[1]->GetDict().FindInt("b"), 2);
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
