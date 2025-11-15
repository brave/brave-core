// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/near_verifier.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kTestModelKey[] = "near-test-model";
constexpr char kTestModelName[] = "near-test-model-name";
constexpr char kTestTurnUuid[] = "test-turn-uuid";
constexpr char kTestLogId1[] = "log-id-1";
constexpr char kTestLogId2[] = "log-id-2";
constexpr char kTestLogId3[] = "log-id-3";
constexpr base::TimeDelta kTestFastForwardInterval = base::Seconds(2);

mojom::ModelPtr CreateNEARTestModel() {
  auto model = mojom::Model::New();
  model->key = kTestModelKey;
  model->display_name = "Test NEAR Model";
  model->is_near_model = true;

  auto leo_options = mojom::LeoModelOptions::New();
  leo_options->name = kTestModelName;
  leo_options->display_maker = "Test";
  leo_options->category = mojom::ModelCategory::CHAT;
  leo_options->access = mojom::ModelAccess::BASIC;
  leo_options->max_associated_content_length = 10000;
  leo_options->long_conversation_warning_character_limit = 9000;

  model->options =
      mojom::ModelOptions::NewLeoModelOptions(std::move(leo_options));

  return model;
}

mojom::ConversationTurnPtr CreateTurnWithCompletionEvents(
    std::vector<std::string> log_ids) {
  std::vector<mojom::ConversationEntryEventPtr> events;

  for (const auto& log_id : log_ids) {
    auto completion_event = mojom::CompletionEvent::New("partial text", log_id);
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        std::move(completion_event));
    events.push_back(std::move(event));
  }

  return mojom::ConversationTurn::New(
      kTestTurnUuid, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "Test response", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::move(events), base::Time::Now(),
      std::nullopt /* edits */, std::nullopt /* uploaded_files */,
      nullptr /* skill */, false /* from_brave_search_SERP */, kTestModelKey,
      std::nullopt /* is_near_verified */);
}

}  // namespace

class AIChatNEARVerifierTest : public testing::Test {
 public:
  void SetUp() override {
    test_model_ = CreateNEARTestModel();

    test_url_loader_factory_.SetInterceptor(base::BindRepeating(
        &AIChatNEARVerifierTest::HandleRequest, base::Unretained(this)));

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);

    verifier_ = std::make_unique<NEARVerifier>(
        shared_url_loader_factory_,
        base::BindRepeating(&AIChatNEARVerifierTest::GetModel,
                            base::Unretained(this)),
        base::BindRepeating(&AIChatNEARVerifierTest::OnVerificationComplete,
                            base::Unretained(this)));
  }

  void TearDown() override { verifier_.reset(); }

 protected:
  void SetVerificationStatus(const std::string& log_id,
                             std::optional<bool> verified) {
    auto path = base::StrCat(
        {"/v1/near-result-verification/", kTestModelName, "/", log_id});
    pending_responses_[path] = verified;
  }

  const mojom::Model* GetModel(std::string_view model_key) {
    if (model_key == kTestModelKey) {
      return test_model_.get();
    }
    return nullptr;
  }

  void HandleRequest(const network::ResourceRequest& request) {
    request_count_++;

    auto url = request.url;
    auto path = url.path();

    if (return_server_error_) {
      test_url_loader_factory_.AddResponse(url.spec(), "",
                                           net::HTTP_INTERNAL_SERVER_ERROR);
      return_server_error_ = false;
      return;
    }

    const auto* pending_response = base::FindOrNull(pending_responses_, path);
    if (!pending_response) {
      ADD_FAILURE() << "Unexpected request for: " << path;
      test_url_loader_factory_.AddResponse(url.spec(), "", net::HTTP_NOT_FOUND);
      return;
    }

    base::Value::Dict dict;
    base::Value status;
    if (pending_response->has_value()) {
      status = base::Value(**pending_response);
    }
    dict.Set("status", std::move(status));
    std::string body;
    base::JSONWriter::Write(dict, &body);

    test_url_loader_factory_.AddResponse(url.spec(), body);
  }

  void OnVerificationComplete(const std::string& turn_uuid, bool verified) {
    EXPECT_EQ(turn_uuid, kTestTurnUuid);
    EXPECT_FALSE(verification_result_.has_value());
    verification_result_ = verified;
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  mojom::ModelPtr test_model_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NEARVerifier> verifier_;

  std::optional<bool> verification_result_;
  base::flat_map<std::string, std::optional<bool>> pending_responses_;
  bool return_server_error_ = false;
  size_t request_count_ = 0;
};

TEST_F(AIChatNEARVerifierTest, SingleCompletionEvent_Verified) {
  SetVerificationStatus(kTestLogId1, true);

  auto turn = CreateTurnWithCompletionEvents({kTestLogId1});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_EQ(request_count_, 1u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, SingleCompletionEvent_NotVerified) {
  SetVerificationStatus(kTestLogId1, false);

  auto turn = CreateTurnWithCompletionEvents({kTestLogId1});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_EQ(request_count_, 1u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_FALSE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, SingleCompletionEvent_PendingThenVerified) {
  SetVerificationStatus(kTestLogId1, std::nullopt);

  auto turn = CreateTurnWithCompletionEvents({kTestLogId1});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  SetVerificationStatus(kTestLogId1, true);
  task_environment_.FastForwardBy(kTestFastForwardInterval * 2);

  EXPECT_GE(request_count_, 2u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, SingleCompletionEvent_ServerErrorThenVerified) {
  return_server_error_ = true;
  SetVerificationStatus(kTestLogId1, std::nullopt);

  auto turn = CreateTurnWithCompletionEvents({kTestLogId1});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  SetVerificationStatus(kTestLogId1, true);
  task_environment_.FastForwardBy(base::Seconds(5));

  // No result yet because we back off for 10 seconds total
  EXPECT_FALSE(verification_result_.has_value());

  task_environment_.FastForwardBy(base::Seconds(6));

  EXPECT_GE(request_count_, 2u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, SingleCompletionEvent_Expired) {
  SetVerificationStatus(kTestLogId1, std::nullopt);

  auto turn = CreateTurnWithCompletionEvents({kTestLogId1});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  // Fast forward to expiration
  task_environment_.FastForwardBy(kTestFastForwardInterval + base::Minutes(1));
  const size_t request_count_before_expiration = request_count_;
  EXPECT_GT(request_count_, 10u);

  // Fast forward past expiration
  task_environment_.FastForwardBy(base::Minutes(1));

  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_FALSE(*verification_result_);
  EXPECT_EQ(request_count_, request_count_before_expiration);
}

TEST_F(AIChatNEARVerifierTest, MultipleCompletionEvents_AllVerified) {
  SetVerificationStatus(kTestLogId1, true);
  SetVerificationStatus(kTestLogId2, true);
  SetVerificationStatus(kTestLogId3, true);

  auto turn =
      CreateTurnWithCompletionEvents({kTestLogId1, kTestLogId2, kTestLogId3});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_EQ(request_count_, 3u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, MultipleCompletionEvents_OneNotVerified) {
  SetVerificationStatus(kTestLogId1, true);
  SetVerificationStatus(kTestLogId2, false);
  SetVerificationStatus(kTestLogId3, true);

  auto turn =
      CreateTurnWithCompletionEvents({kTestLogId1, kTestLogId2, kTestLogId3});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_EQ(request_count_, 3u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_FALSE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest,
       MultipleCompletionEvents_PendingThenAllVerified) {
  SetVerificationStatus(kTestLogId1, std::nullopt);
  SetVerificationStatus(kTestLogId2, true);
  SetVerificationStatus(kTestLogId3, std::nullopt);

  auto turn =
      CreateTurnWithCompletionEvents({kTestLogId1, kTestLogId2, kTestLogId3});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  SetVerificationStatus(kTestLogId1, true);
  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  SetVerificationStatus(kTestLogId3, true);
  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_GE(request_count_, 5u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest,
       MultipleCompletionEvents_DifferentRetryIntervals) {
  SetVerificationStatus(kTestLogId1, std::nullopt);
  SetVerificationStatus(kTestLogId2, std::nullopt);
  SetVerificationStatus(kTestLogId3, true);

  return_server_error_ = true;

  auto turn =
      CreateTurnWithCompletionEvents({kTestLogId1, kTestLogId2, kTestLogId3});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  SetVerificationStatus(kTestLogId1, true);
  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  SetVerificationStatus(kTestLogId2, true);
  task_environment_.FastForwardBy(base::Seconds(20));

  EXPECT_GE(request_count_, 5u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, MultipleCompletionEvents_Expired) {
  SetVerificationStatus(kTestLogId1, std::nullopt);
  SetVerificationStatus(kTestLogId2, std::nullopt);
  SetVerificationStatus(kTestLogId3, true);

  auto turn =
      CreateTurnWithCompletionEvents({kTestLogId1, kTestLogId2, kTestLogId3});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);
  EXPECT_FALSE(verification_result_.has_value());

  // Fast forward to expiration
  task_environment_.FastForwardBy(kTestFastForwardInterval + base::Minutes(1));
  const size_t request_count_before_expiration = request_count_;

  // Fast forward past expiration
  task_environment_.FastForwardBy(base::Minutes(1));

  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_FALSE(*verification_result_);
  EXPECT_EQ(request_count_, request_count_before_expiration);
}

TEST_F(AIChatNEARVerifierTest, DuplicateLogIds_OnlyVerifyOnce) {
  SetVerificationStatus(kTestLogId1, true);

  auto turn =
      CreateTurnWithCompletionEvents({kTestLogId1, kTestLogId1, kTestLogId1});
  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_EQ(request_count_, 1u);
  ASSERT_TRUE(verification_result_.has_value());
  EXPECT_TRUE(*verification_result_);
}

TEST_F(AIChatNEARVerifierTest, NonNEARModel_NoVerification) {
  auto turn = CreateTurnWithCompletionEvents({kTestLogId1});
  turn->model_key = "non-near-model";

  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_FALSE(verification_result_.has_value());
  EXPECT_EQ(request_count_, 0u);
}

TEST_F(AIChatNEARVerifierTest, NoLogIds_NoVerification) {
  auto turn = CreateTurnWithCompletionEvents({});

  verifier_->MaybeVerifyConversationEntry(*turn);

  task_environment_.FastForwardBy(kTestFastForwardInterval);

  EXPECT_FALSE(verification_result_.has_value());
  EXPECT_EQ(request_count_, 0u);
}

}  // namespace ai_chat
