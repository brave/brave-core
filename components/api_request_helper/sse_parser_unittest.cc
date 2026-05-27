/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/sse_parser.h"

#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
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
  void OnEvent(ValueOrError result) {
    results_.push_back(std::move(result));
    if (!parser_->IsDecoding()) {
      run_loop_->Quit();
    }
  }

  void Process(std::string_view chunk) {
    run_loop_ = std::make_unique<base::RunLoop>();
    parser_->Process(chunk);
    run_loop_->Run();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<base::RunLoop> run_loop_;
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
  EXPECT_EQ(*results_[0]->GetDict().FindInt("a"), 1);
  EXPECT_EQ(*results_[1]->GetDict().FindInt("b"), 2);
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

TEST_F(SSEParserTest, MixedLineEndings) {
  Process("data: {\"a\":1}\r\ndata: {\"b\":2}\r");
  ASSERT_EQ(results_.size(), 2u);
  EXPECT_EQ(*results_[0]->GetDict().FindInt("a"), 1);
  EXPECT_EQ(*results_[1]->GetDict().FindInt("b"), 2);
}

}  // namespace api_request_helper
