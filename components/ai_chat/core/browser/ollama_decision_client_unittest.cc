// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/ollama_decision_client.h"

#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kEndpoint[] = "http://localhost:11434/v1/systemone";

// A System One response for one sentence. |fact| is p(true).
std::string SentenceResponse(double fact) {
  return base::StringPrintf(
      R"({"answers": {
        "fact": {"type": "noul", "noul": %f},
        "safety": {"type": "choice", "choice": "ok", "probabilities": {
          "ok": 0.9, "sensitive": 0.04, "instruction": 0.02,
          "short_lived": 0.02, "not_about_user": 0.02}},
        "category": {"type": "choice", "choice": "preference",
          "probabilities": {"personal_fact": 0.1, "preference": 0.85,
                            "topic": 0.05}},
        "temporary": {"type": "noul", "noul": 0.1}}})",
      fact);
}

}  // namespace

class OllamaDecisionClientTest : public testing::Test {
 protected:
  void SetUp() override {
    client_ = std::make_unique<OllamaDecisionClient>(
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_),
        GURL(kEndpoint), "clef-flash:9b");
    // Answer by the state of the request, so the answers come in any order.
    url_loader_factory_.SetInterceptor(
        base::BindLambdaForTesting([&](const network::ResourceRequest& r) {
          last_body_ = std::string(r.request_body->elements()
                                       ->at(0)
                                       .As<network::DataElementBytes>()
                                       .AsStringPiece());
          auto body = base::JSONReader::ReadDict(
              last_body_, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
          ASSERT_TRUE(body);
          EXPECT_EQ(*body->FindString("model"), "clef-flash:9b");
          const std::string state = *body->FindString("state");
          states_.push_back(state);
          std::string response = state == "fail"    ? "not json"
                                 : state == "first" ? SentenceResponse(0.9)
                                                    : SentenceResponse(0.2);
          url_loader_factory_.AddResponse(kEndpoint, response);
        }));
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<OllamaDecisionClient> client_;
  std::vector<std::string> states_;
  std::string last_body_;
};

TEST_F(OllamaDecisionClientTest, SentenceDecisionsKeepTheOrder) {
  base::test::TestFuture<std::optional<std::vector<SentenceDecisions>>> future;
  client_->AskSentenceDecisions({"first", "second"}, future.GetCallback());
  auto decisions = future.Take();

  ASSERT_TRUE(decisions);
  ASSERT_EQ(decisions->size(), 2u);
  EXPECT_DOUBLE_EQ((*decisions)[0].fact.at(true), 0.9);
  EXPECT_DOUBLE_EQ((*decisions)[1].fact.at(true), 0.2);
  EXPECT_DOUBLE_EQ((*decisions)[0].safety.at(SafetyAnswer::kOk), 0.9);
  EXPECT_DOUBLE_EQ(
      (*decisions)[0].category.at(LearnedMemoryCategory::kPreference), 0.85);
  // One request for each sentence.
  EXPECT_EQ(states_.size(), 2u);
}

TEST_F(OllamaDecisionClientTest, RequestKeepsTheOrderOfQuestionsAndOptions) {
  // The order changes the probabilities that the model gives, and a
  // dictionary sorts its keys. So the request is built as text.
  base::test::TestFuture<std::optional<std::vector<SentenceDecisions>>> future;
  client_->AskSentenceDecisions({"first"}, future.GetCallback());
  ASSERT_TRUE(future.Take());

  auto before = [&](std::string_view a, std::string_view b) {
    size_t pos_a = last_body_.find(a);
    size_t pos_b = last_body_.find(b);
    return pos_a != std::string::npos && pos_b != std::string::npos &&
           pos_a < pos_b;
  };
  EXPECT_TRUE(before("\"fact\":{", "\"category\":{"));
  EXPECT_TRUE(before("\"category\":{", "\"temporary\":{"));
  EXPECT_TRUE(before("\"temporary\":{", "\"safety\":{"));
  EXPECT_TRUE(before("\"sensitive\":", "\"instruction\":"));
  EXPECT_TRUE(before("\"short_lived\":", "\"not_about_user\":"));
  EXPECT_TRUE(before("\"not_about_user\":", "\"ok\":"));
}

TEST_F(OllamaDecisionClientTest, BadResponseFailsTheRequest) {
  base::test::TestFuture<std::optional<std::vector<SentenceDecisions>>> future;
  client_->AskSentenceDecisions({"first", "fail"}, future.GetCallback());
  EXPECT_FALSE(future.Take());

  // A missing answer also fails.
  base::test::TestFuture<std::optional<AnswerProbabilities<bool>>> gate;
  client_->AskGate("first", gate.GetCallback());
  EXPECT_FALSE(gate.Take());
}

}  // namespace ai_chat
