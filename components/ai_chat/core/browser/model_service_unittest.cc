// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_service.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/metrics/field_trial_params.h"
#include "base/numerics/safe_math.h"
#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {
using ::testing::_;
using ::testing::NiceMock;

class MockModelServiceObserver : public ModelService::Observer {
 public:
  MockModelServiceObserver() = default;
  ~MockModelServiceObserver() override = default;

  void Observe(ModelService* model_service) {
    models_observer_.Observe(model_service);
  }

  MOCK_METHOD(void,
              OnDefaultModelChanged,
              (const std::string&, const std::string&),
              (override));

 private:
  base::ScopedObservation<ModelService, ModelService::Observer>
      models_observer_{this};
};

}  // namespace

class ModelServiceTest : public ::testing::Test {
 public:
  void SetUp() override {
    OSCryptMocker::SetUp();
    prefs::RegisterProfilePrefs(pref_service_.registry());
    prefs::RegisterProfilePrefsForMigration(pref_service_.registry());
    ModelService::RegisterProfilePrefs(pref_service_.registry());
    observer_ = std::make_unique<NiceMock<MockModelServiceObserver>>();
  }

  ModelService* GetService() {
    if (!service_) {
      service_ = std::make_unique<ModelService>(&pref_service_);
      observer_->Observe(service_.get());
    }
    return service_.get();
  }

  void TearDown() override {
    OSCryptMocker::TearDown();
    observer_.reset();
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NiceMock<MockModelServiceObserver>> observer_;

 private:
  std::unique_ptr<ModelService> service_;
};

class ModelServiceTestWithDifferentPremiumModel : public ModelServiceTest {
 public:
  ModelServiceTestWithDifferentPremiumModel() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kAIChat,
        {
            {features::kAIModelsDefaultKey.name, "chat-leo-expanded"},
            {features::kAIModelsPremiumDefaultKey.name, "claude-3-sonnet"},
        });
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

class ModelServiceTestWithSamePremiumModel : public ModelServiceTest {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kAIChat,
        {
            {features::kAIModelsDefaultKey.name, "chat-leo-expanded"},
            {features::kAIModelsPremiumDefaultKey.name, "chat-leo-expanded"},
        });
    ModelServiceTest::SetUp();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(ModelServiceTest, MigrateOldClaudeDefaultModelKey) {
  // Set default to the old key for claude
  pref_service_.SetString("brave.ai_chat.default_model_key",
                          "chat-claude-instant");
  // Call Migrate even though it shouldn't touch this pref value, precisely
  // to test that it doesn't interfere with the translation from old claude to
  // new claude.
  ModelService::MigrateProfilePrefs(&pref_service_);
  // Verify uses non-premium version
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-haiku");
  // Verify uses premium version
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-claude-haiku", "chat-claude-sonnet"))
      .Times(1);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-sonnet");
}

TEST_F(ModelServiceTest, MigrateOldClaudeDefaultModelKey_OnlyOnce) {
  // Set default to the old key for claude
  pref_service_.SetString("brave.ai_chat.default_model_key",
                          "chat-claude-instant");
  // Call Migrate even though it shouldn't touch this pref value, precisely
  // to test that it doesn't interfere with the translation from old claude to
  // new claude.
  ModelService::MigrateProfilePrefs(&pref_service_);
  // Verify uses non-premium version
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-haiku");
  EXPECT_CALL(*observer_, OnDefaultModelChanged(_, _)).Times(0);
  // Verify keeps non-premium version
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Inactive);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-haiku");
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-haiku");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTestWithDifferentPremiumModel,
       MigrateToPremiumDefaultModel) {
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-leo-expanded");
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-leo-expanded", "claude-3-sonnet"))
      .Times(1);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "claude-3-sonnet");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTestWithDifferentPremiumModel,
       MigrateToPremiumDefaultModel_UserModified) {
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-leo-expanded");
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-leo-expanded", "chat-basic"))
      .Times(1);
  GetService()->SetDefaultModelKey("chat-basic");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
  EXPECT_CALL(*observer_, OnDefaultModelChanged(_, _)).Times(0);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTestWithSamePremiumModel,
       MigrateToPremiumDefaultModel_None) {
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-leo-expanded");
  EXPECT_CALL(*observer_, OnDefaultModelChanged(_, _)).Times(0);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-leo-expanded");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTest, ChangeOldDefaultKey) {
  GetService()->SetDefaultModelKeyWithoutValidationForTesting("chat-default");
  ModelService::MigrateProfilePrefs(&pref_service_);

  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
}

TEST_F(ModelServiceTest, AddAndModifyCustomModel) {
  static constexpr char kRequestName[] = "request_name";
  static constexpr char kModelSystemPrompt[] = "model_system_prompt";
  static const GURL kEndpoint = GURL("http://brave.com");
  static constexpr char kAPIKey[] = "foo_api_key";
  static constexpr char kDisplayName[] = "Custom display name";

  {
    mojom::ModelPtr model = mojom::Model::New();
    model->display_name = kDisplayName;
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New(kRequestName, 0, 0, 0,
                                       kModelSystemPrompt, kEndpoint, kAPIKey));

    GetService()->AddCustomModel(std::move(model));
  }

  const std::vector<mojom::ModelPtr>& models = GetService()->GetModels();

  EXPECT_EQ(models.back()->display_name, kDisplayName);
  EXPECT_EQ(
      models.back()->options->get_custom_model_options()->model_request_name,
      kRequestName);
  EXPECT_EQ(
      models.back()->options->get_custom_model_options()->model_system_prompt,
      kModelSystemPrompt);
  EXPECT_EQ(models.back()->options->get_custom_model_options()->endpoint.spec(),
            kEndpoint.spec());
  EXPECT_EQ(models.back()->options->get_custom_model_options()->api_key,
            kAPIKey);
}

TEST_F(ModelServiceTest, ChangeDefaultModelKey_GoodKey) {
  GetService()->SetDefaultModelKey("chat-basic");
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-basic", "chat-leo-expanded"))
      .Times(1);
  GetService()->SetDefaultModelKey("chat-leo-expanded");
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-leo-expanded");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTest, ChangeDefaultModelKey_IncorrectKey) {
  GetService()->SetDefaultModelKey("chat-basic");
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  EXPECT_CALL(*observer_, OnDefaultModelChanged(_, _)).Times(0);
  GetService()->SetDefaultModelKey("bad-key");
  // Default model key should not change if the key is invalid.
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTest, SetAssociatedContentLengthMetrics_CustomModel) {
  // Setup a custom model with no valid context size
  mojom::CustomModelOptionsPtr custom_options =
      mojom::CustomModelOptions::New();
  custom_options->context_size = 0;  // Invalid context size

  mojom::Model custom_model;
  custom_model.options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(custom_options));

  // Set associated content length metrics
  GetService()->SetAssociatedContentLengthMetrics(custom_model);

  // Validate that default context size is set
  EXPECT_EQ(custom_model.options->get_custom_model_options()->context_size,
            kDefaultCustomModelContextSize);

  // Validate that max_associated_content_length is calculated correctly
  size_t expected_content_length =
      GetService()->CalcuateMaxAssociatedContentLengthForModel(custom_model);
  EXPECT_EQ(custom_model.options->get_custom_model_options()
                ->max_associated_content_length,
            expected_content_length);

  // Validate that long_conversation_warning_character_limit is calculated
  // correctly
  uint32_t expected_warning_limit = static_cast<uint32_t>(
      expected_content_length * kMaxContentLengthThreshold);
  EXPECT_EQ(custom_model.options->get_custom_model_options()
                ->long_conversation_warning_character_limit,
            expected_warning_limit);
}

TEST_F(ModelServiceTest, SetAssociatedContentLengthMetrics_ValidContextSize) {
  // Setup a custom model with a valid context size
  static constexpr size_t kContextSize = 5000;
  mojom::CustomModelOptionsPtr custom_options =
      mojom::CustomModelOptions::New();
  custom_options->context_size = kContextSize;

  mojom::Model custom_model;
  custom_model.options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(custom_options));

  // Set associated content length metrics
  GetService()->SetAssociatedContentLengthMetrics(custom_model);

  // Validate that the provided context size is retained
  EXPECT_EQ(custom_model.options->get_custom_model_options()->context_size,
            kContextSize);

  // Validate that max_associated_content_length is calculated correctly
  size_t expected_content_length =
      GetService()->CalcuateMaxAssociatedContentLengthForModel(custom_model);
  EXPECT_EQ(custom_model.options->get_custom_model_options()
                ->max_associated_content_length,
            expected_content_length);

  // Validate long_conversation_warning_character_limit calculation
  base::CheckedNumeric<size_t> checked_warning_limit = base::CheckMul(
      expected_content_length, static_cast<double>(kMaxContentLengthThreshold));

  ASSERT_TRUE(checked_warning_limit.IsValid());

  size_t expected_warning_limit = checked_warning_limit.ValueOrDie();

  EXPECT_EQ(custom_model.options->get_custom_model_options()
                ->long_conversation_warning_character_limit,
            expected_warning_limit);
}

TEST_F(ModelServiceTest,
       CalcuateMaxAssociatedContentLengthForModel_CustomModel) {
  // Setup a custom model with a valid context size
  mojom::CustomModelOptionsPtr custom_options =
      mojom::CustomModelOptions::New();
  custom_options->context_size = 5000;

  mojom::Model custom_model;
  custom_model.options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(custom_options));

  // Calculate max associated content length
  size_t max_content_length =
      GetService()->CalcuateMaxAssociatedContentLengthForModel(custom_model);

  // Validate that max content length is correct
  static constexpr uint32_t reserved_tokens =
      kReservedTokensForMaxNewTokens + kReservedTokensForPrompt;

  static constexpr size_t expected_content_length =
      (5000 - reserved_tokens) * kDefaultCharsPerToken;

  EXPECT_EQ(max_content_length, expected_content_length);
}

TEST_F(ModelServiceTest, CalcuateMaxAssociatedContentLengthForModel_LeoModel) {
  // Setup a leo model with predefined page content length
  static constexpr size_t expected_content_length = 10'000;

  mojom::LeoModelOptionsPtr leo_options = mojom::LeoModelOptions::New();
  leo_options->max_associated_content_length = expected_content_length;

  mojom::Model leo_model;
  leo_model.options =
      mojom::ModelOptions::NewLeoModelOptions(std::move(leo_options));

  // Calculate max associated content length
  size_t max_content_length =
      GetService()->CalcuateMaxAssociatedContentLengthForModel(leo_model);

  // Validate that the predefined value is returned
  EXPECT_EQ(max_content_length, expected_content_length);
}

}  // namespace ai_chat
