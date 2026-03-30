// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_service.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "base/metrics/field_trial_params.h"
#include "base/numerics/safe_math.h"
#include "base/scoped_observation.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
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
  MOCK_METHOD(void, OnModelListUpdated, (), (override));
  MOCK_METHOD(void, OnModelRemoved, (const std::string&), (override));

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
            {features::kAIModelsDefaultKey.name, "chat-basic"},
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
            {features::kAIModelsDefaultKey.name, "chat-basic"},
            {features::kAIModelsPremiumDefaultKey.name, "chat-basic"},
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
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-basic", "claude-3-sonnet"))
      .Times(1);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "claude-3-sonnet");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTestWithDifferentPremiumModel,
       MigrateToPremiumDefaultModel_UserModified) {
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-basic", "chat-claude-haiku"))
      .Times(1);
  GetService()->SetDefaultModelKey("chat-claude-haiku");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
  EXPECT_CALL(*observer_, OnDefaultModelChanged(_, _)).Times(0);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-haiku");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTestWithSamePremiumModel,
       MigrateToPremiumDefaultModel_None) {
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  EXPECT_CALL(*observer_, OnDefaultModelChanged(_, _)).Times(0);
  GetService()->OnPremiumStatus(mojom::PremiumStatus::Active);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTest, ChangeOldDefaultKey) {
  constexpr std::array<const char*, 2> old_keys = {
      "chat-default",
      "chat-leo-expanded",
  };

  for (const char* old_key : old_keys) {
    GetService()->SetDefaultModelKeyWithoutValidationForTesting(old_key);
    ModelService::MigrateProfilePrefs(&pref_service_);
    EXPECT_EQ(GetService()->GetDefaultModelKey(),
              features::kAIModelsDefaultKey.Get())
        << "Failed to migrate key: " << old_key;
  }
}

TEST_F(ModelServiceTest, AddAndModifyCustomModel) {
  static constexpr char kRequestName[] = "request_name";
  static constexpr char kModelSystemPrompt[] = "model_system_prompt";
  static constexpr char kAPIKey[] = "foo_api_key";
  static constexpr char kDisplayName[] = "Custom display name";
  const GURL endpoint = GURL("http://brave.com");

  {
    mojom::ModelPtr model = mojom::Model::New();
    model->display_name = kDisplayName;
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New(kRequestName, 0, 0, 0,
                                       kModelSystemPrompt, endpoint, kAPIKey));

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
            endpoint.spec());
  EXPECT_EQ(models.back()->options->get_custom_model_options()->api_key,
            kAPIKey);
}

TEST_F(ModelServiceTest, ChangeDefaultModelKey_GoodKey) {
  GetService()->SetDefaultModelKey("chat-basic");
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-basic");
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged("chat-basic", "chat-claude-haiku"))
      .Times(1);
  GetService()->SetDefaultModelKey("chat-claude-haiku");
  EXPECT_EQ(GetService()->GetDefaultModelKey(), "chat-claude-haiku");
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

TEST_F(ModelServiceTest, GetLeoModelKeyByName_And_GetLeoModelNameByKey) {
  auto& models = GetService()->GetModels();
  for (const auto& model : models) {
    ASSERT_TRUE(model->options->is_leo_model_options());
    EXPECT_EQ(GetService()->GetLeoModelKeyByName(
                  model->options->get_leo_model_options()->name),
              model->key);
    EXPECT_EQ(GetService()->GetLeoModelNameByKey(model->key),
              model->options->get_leo_model_options()->name);
  }

  // Test with an invalid model name or key
  auto key = GetService()->GetLeoModelKeyByName("nonexistent-model");
  EXPECT_FALSE(key.has_value());
  auto name = GetService()->GetLeoModelNameByKey("nonexistent-key");
  EXPECT_FALSE(name.has_value());
}

TEST_F(ModelServiceTest, LeoModelLookupSkipsCustomModels) {
  mojom::ModelPtr custom = mojom::Model::New();
  custom->display_name = "Custom Model";
  custom->options =
      mojom::ModelOptions::NewCustomModelOptions(mojom::CustomModelOptions::New(
          "custom-req", 0, 0, 0, "", GURL("http://example.com"), ""));
  GetService()->AddCustomModel(std::move(custom));

  auto custom_models = GetService()->GetCustomModels();
  ASSERT_EQ(custom_models.size(), 1u);
  std::string custom_key = custom_models[0]->key;

  // GetLeoModelNameByKey should return nullopt for a custom model key
  auto name = GetService()->GetLeoModelNameByKey(custom_key);
  EXPECT_FALSE(name.has_value());

  // GetLeoModelKeyByName should not match custom model request names
  auto key = GetService()->GetLeoModelKeyByName("custom-req");
  EXPECT_FALSE(key.has_value());

  // Leo lookups should still work with custom models present
  auto& models = GetService()->GetModels();
  for (const auto& model : models) {
    if (!model->options->is_leo_model_options()) {
      continue;
    }
    EXPECT_EQ(GetService()->GetLeoModelKeyByName(
                  model->options->get_leo_model_options()->name),
              model->key);
    EXPECT_EQ(GetService()->GetLeoModelNameByKey(model->key),
              model->options->get_leo_model_options()->name);
  }
}

TEST_F(ModelServiceTest, DeleteCustomModelsByEndpoint) {
  const GURL endpoint1 = GURL("http://example.com");
  const GURL endpoint2 = GURL("http://other.com");

  // Add multiple custom models with different endpoints
  std::string model1_key;
  std::string model2_key;
  {
    mojom::ModelPtr model1 = mojom::Model::New();
    model1->display_name = "Model 1";
    model1->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model1", 0, 0, 0, "", endpoint1, ""));
    GetService()->AddCustomModel(std::move(model1));

    mojom::ModelPtr model2 = mojom::Model::New();
    model2->display_name = "Model 2";
    model2->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model2", 0, 0, 0, "", endpoint1, ""));
    GetService()->AddCustomModel(std::move(model2));

    mojom::ModelPtr model3 = mojom::Model::New();
    model3->display_name = "Model 3";
    model3->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model3", 0, 0, 0, "", endpoint2, ""));
    GetService()->AddCustomModel(std::move(model3));

    auto custom_models = GetService()->GetCustomModels();
    ASSERT_EQ(custom_models.size(), 3u);
    model1_key = custom_models[0]->key;
    model2_key = custom_models[1]->key;
  }

  {
    auto custom_models = GetService()->GetCustomModels();
    EXPECT_EQ(custom_models.size(), 3u);
  }

  // Expect OnModelRemoved to be called for model1 and model2
  EXPECT_CALL(*observer_, OnModelRemoved(model1_key)).Times(1);
  EXPECT_CALL(*observer_, OnModelRemoved(model2_key)).Times(1);
  EXPECT_CALL(*observer_, OnModelListUpdated()).Times(1);

  // Delete all models with endpoint1
  GetService()->MaybeDeleteCustomModels(base::BindLambdaForTesting(
      [&endpoint1](const base::DictValue& model_dict) {
        const std::string* endpoint_str =
            model_dict.FindString(kCustomModelItemEndpointUrlKey);
        return endpoint_str && GURL(*endpoint_str) == endpoint1;
      }));

  auto custom_models = GetService()->GetCustomModels();
  EXPECT_EQ(custom_models.size(), 1u);
  EXPECT_EQ(custom_models[0]->display_name, "Model 3");
  EXPECT_EQ(custom_models[0]->options->get_custom_model_options()->endpoint,
            endpoint2);

  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTest, DeleteCustomModelByNameAndEndpoint) {
  const GURL endpoint = GURL("http://example.com");

  // Add multiple custom models with the same endpoint
  std::string model1_key;
  {
    mojom::ModelPtr model1 = mojom::Model::New();
    model1->display_name = "Model 1";
    model1->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model1", 0, 0, 0, "", endpoint, ""));
    GetService()->AddCustomModel(std::move(model1));

    mojom::ModelPtr model2 = mojom::Model::New();
    model2->display_name = "Model 2";
    model2->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model2", 0, 0, 0, "", endpoint, ""));
    GetService()->AddCustomModel(std::move(model2));

    auto custom_models = GetService()->GetCustomModels();
    ASSERT_EQ(custom_models.size(), 2u);
    model1_key = custom_models[0]->key;
  }

  {
    auto custom_models = GetService()->GetCustomModels();
    EXPECT_EQ(custom_models.size(), 2u);
  }

  // Expect OnModelRemoved to be called for model1 only
  EXPECT_CALL(*observer_, OnModelRemoved(model1_key)).Times(1);
  EXPECT_CALL(*observer_, OnModelListUpdated()).Times(1);

  // Delete only model1
  GetService()->MaybeDeleteCustomModels(base::BindLambdaForTesting(
      [&endpoint](const base::DictValue& model_dict) {
        const std::string* endpoint_str =
            model_dict.FindString(kCustomModelItemEndpointUrlKey);
        const std::string* model_name =
            model_dict.FindString(kCustomModelItemModelKey);
        return endpoint_str && model_name && GURL(*endpoint_str) == endpoint &&
               *model_name == "model1";
      }));

  auto custom_models = GetService()->GetCustomModels();
  EXPECT_EQ(custom_models.size(), 1u);
  EXPECT_EQ(custom_models[0]->display_name, "Model 2");
  EXPECT_EQ(
      custom_models[0]->options->get_custom_model_options()->model_request_name,
      "model2");

  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(ModelServiceTest, DeleteCustomModelsByEndpoint_WithDefaultModel) {
  const GURL endpoint = GURL("http://example.com");

  // Add a custom model
  std::string custom_model_key;
  {
    mojom::ModelPtr model = mojom::Model::New();
    model->display_name = "Custom Model";
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model1", 0, 0, 0, "", endpoint, ""));
    GetService()->AddCustomModel(std::move(model));

    auto custom_models = GetService()->GetCustomModels();
    ASSERT_EQ(custom_models.size(), 1u);
    custom_model_key = custom_models[0]->key;
  }

  // Set the custom model as default
  GetService()->SetDefaultModelKey(custom_model_key);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), custom_model_key);

  // Expect observers to be called when default model is removed
  const std::string expected_default = features::kAIModelsDefaultKey.Get();
  EXPECT_CALL(*observer_,
              OnDefaultModelChanged(custom_model_key, expected_default))
      .Times(1);
  EXPECT_CALL(*observer_, OnModelRemoved(custom_model_key)).Times(1);
  // Expect OnModelListUpdated to be called when InitModels is called
  EXPECT_CALL(*observer_, OnModelListUpdated()).Times(1);

  // Delete the model
  GetService()->MaybeDeleteCustomModels(base::BindLambdaForTesting(
      [&endpoint](const base::DictValue& model_dict) {
        const std::string* endpoint_str =
            model_dict.FindString(kCustomModelItemEndpointUrlKey);
        return endpoint_str && GURL(*endpoint_str) == endpoint;
      }));

  // Verify OnDefaultModelChanged was called
  testing::Mock::VerifyAndClearExpectations(observer_.get());

  // Default model should be reset to the platform default
  EXPECT_NE(GetService()->GetDefaultModelKey(), custom_model_key);
  EXPECT_EQ(GetService()->GetDefaultModelKey(), expected_default);
}

TEST_F(ModelServiceTest, GetCustomModels) {
  // Initially should be empty
  {
    auto custom_models = GetService()->GetCustomModels();
    EXPECT_EQ(custom_models.size(), 0u);
  }

  // Leo models should exist
  size_t initial_model_count = GetService()->GetModels().size();
  EXPECT_GT(initial_model_count, 0u);

  // Add a custom model
  const GURL endpoint = GURL("http://example.com");
  {
    mojom::ModelPtr model = mojom::Model::New();
    model->display_name = "Custom Model";
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New("model1", 0, 0, 0, "", endpoint, ""));
    GetService()->AddCustomModel(std::move(model));
  }

  // GetCustomModels should return only custom models
  {
    auto custom_models = GetService()->GetCustomModels();
    EXPECT_EQ(custom_models.size(), 1u);
    EXPECT_TRUE(custom_models[0]->options->is_custom_model_options());
  }

  // GetModels should return both Leo and custom models
  EXPECT_EQ(GetService()->GetModels().size(), initial_model_count + 1);
}

// Remote Models Tests

class ModelServiceRemoteModelsTest : public testing::Test {
 public:
  ModelServiceRemoteModelsTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

  void SetUp() override {
    OSCryptMocker::SetUp();
    prefs::RegisterProfilePrefs(pref_service_.registry());
    prefs::RegisterProfilePrefsForMigration(pref_service_.registry());
    ModelService::RegisterProfilePrefs(pref_service_.registry());
    observer_ = std::make_unique<NiceMock<MockModelServiceObserver>>();
  }

  void TearDown() override {
    OSCryptMocker::TearDown();
    observer_.reset();
    service_.reset();
  }

 protected:
  void CreateServiceWithRemoteModels(const std::string& endpoint_url) {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kAIChatRemoteModels,
        {{features::kRemoteModelsEndpoint.name, endpoint_url},
         {features::kRemoteModelsCacheTTLMinutes.name, "60"}});

    service_ = std::make_unique<ModelService>(&pref_service_,
                                              shared_url_loader_factory_);
    observer_->Observe(service_.get());
  }

  void SimulateSuccessfulFetch(const std::string& url,
                               const std::string& json_response) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this, url, json_response](const network::ResourceRequest& request) {
          if (base::StartsWith(request.url.spec(), url)) {
            test_url_loader_factory_.AddResponse(request.url.spec(),
                                                 json_response);
          }
        }));
  }

  void SimulateHTTPError(const std::string& url, int http_code) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this, url, http_code](const network::ResourceRequest& request) {
          if (base::StartsWith(request.url.spec(), url)) {
            test_url_loader_factory_.AddResponse(
                request.url.spec(), "",
                static_cast<net::HttpStatusCode>(http_code));
          }
        }));
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  TestingPrefServiceSimple pref_service_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<NiceMock<MockModelServiceObserver>> observer_;
  std::unique_ptr<ModelService> service_;
};

TEST_F(ModelServiceRemoteModelsTest, UsesStaticModelsWhenEndpointEmpty) {
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      features::kAIChat, {{features::kRemoteModelsEndpoint.name, ""}});

  service_ = std::make_unique<ModelService>(&pref_service_,
                                            shared_url_loader_factory_);

  // Wait for models to be loaded
  EXPECT_TRUE(
      base::test::RunUntil([&]() { return !service_->GetModels().empty(); }));

  // Should use static Leo models
  const auto& models = service_->GetModels();
  EXPECT_GT(models.size(), 0u);

  // Verify it's a Leo model (not remote)
  bool has_leo_model = false;
  for (const auto& model : models) {
    if (model->options->is_leo_model_options()) {
      has_leo_model = true;
      break;
    }
  }
  EXPECT_TRUE(has_leo_model);
}

TEST_F(ModelServiceRemoteModelsTest, FetchesRemoteModelsOnStartup) {
  const char kEndpoint[] = "https://example.com/models";
  const char kRemoteModelsJSON[] = R"({
    "models": [{
      "key": "remote-model-1",
      "display_name": "Remote Model 1",
      "vision_support": true,
      "supports_tools": false,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "remote-model-1-api",
        "display_maker": "Remote Provider",
        "category": "chat",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }]
  })";

  SimulateSuccessfulFetch(kEndpoint, kRemoteModelsJSON);
  CreateServiceWithRemoteModels(kEndpoint);

  // Wait for remote models to be fetched
  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = service_->GetModels();
    return std::any_of(models.begin(), models.end(), [](const auto& m) {
      return m->key == "remote-model-1";
    });
  }));

  // Should have fetched remote models
  const auto& models = service_->GetModels();
  ASSERT_GT(models.size(), 0u);

  // Find the remote model
  bool found_remote_model = false;
  for (const auto& model : models) {
    if (model->key == "remote-model-1") {
      found_remote_model = true;
      EXPECT_EQ("Remote Model 1", model->display_name);
      EXPECT_TRUE(model->vision_support);
      break;
    }
  }
  EXPECT_TRUE(found_remote_model);
}

TEST_F(ModelServiceRemoteModelsTest, FallsBackToStaticOnNetworkError) {
  const char kEndpoint[] = "https://example.com/models";

  SimulateHTTPError(kEndpoint, 500);
  CreateServiceWithRemoteModels(kEndpoint);

  // Wait for fallback to static models
  EXPECT_TRUE(
      base::test::RunUntil([&]() { return !service_->GetModels().empty(); }));

  // Should fall back to static Leo models
  const auto& models = service_->GetModels();
  EXPECT_GT(models.size(), 0u);

  // Verify it's using Leo models (none with "remote" in key)
  bool has_remote_model = false;
  for (const auto& model : models) {
    if (model->key.find("remote") != std::string::npos) {
      has_remote_model = true;
      break;
    }
  }
  EXPECT_FALSE(has_remote_model);
}

TEST_F(ModelServiceRemoteModelsTest, LoadsCachedModelsFromPrefs) {
  const char kEndpoint[] = "https://example.com/models";

  // Manually populate prefs with cached remote models
  base::DictValue cache;
  base::ListValue models_list;

  base::DictValue model1;
  model1.Set("key", "cached-remote-model");
  model1.Set("display_name", "Cached Remote Model");
  model1.Set("vision_support", true);
  model1.Set("supports_tools", false);
  model1.Set("is_suggested_model", true);
  model1.Set("is_near_model", false);

  base::DictValue options1;
  options1.Set("type", "leo");
  options1.Set("name", "cached-remote-model-api");
  options1.Set("display_maker", "Cached Provider");
  options1.Set("category", "chat");
  options1.Set("access", "basic");
  options1.Set("max_associated_content_length", 100000);
  options1.Set("long_conversation_warning_character_limit", 200000);
  model1.Set("options", std::move(options1));

  models_list.Append(std::move(model1));
  cache.Set("models", std::move(models_list));
  cache.Set("last_updated", base::Time::Now().InSecondsFSinceUnixEpoch());
  cache.Set("endpoint_url", kEndpoint);

  pref_service_.SetDict(prefs::kRemoteModelsCache, std::move(cache));

  CreateServiceWithRemoteModels(kEndpoint);

  // Should load cached models immediately without network request
  const auto& models = service_->GetModels();
  ASSERT_GT(models.size(), 0u);

  bool found_cached_model = false;
  for (const auto& model : models) {
    if (model->key == "cached-remote-model") {
      found_cached_model = true;
      EXPECT_EQ("Cached Remote Model", model->display_name);
      break;
    }
  }
  EXPECT_TRUE(found_cached_model);
}

TEST_F(ModelServiceRemoteModelsTest, RefreshRemoteModels) {
  const char kEndpoint[] = "https://example.com/models";
  const char kInitialModelsJSON[] = R"({
    "models": [{
      "key": "initial-model",
      "display_name": "Initial Model",
      "vision_support": true,
      "supports_tools": false,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "initial-model-api",
        "category": "chat",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }]
  })";
  const char kUpdatedModelsJSON[] = R"({
    "models": [{
      "key": "updated-model",
      "display_name": "Updated Model",
      "vision_support": false,
      "supports_tools": true,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "updated-model-api",
        "category": "chat",
        "access": "premium",
        "max_associated_content_length": 150000,
        "long_conversation_warning_character_limit": 300000
      }
    }]
  })";

  SimulateSuccessfulFetch(kEndpoint, kInitialModelsJSON);
  CreateServiceWithRemoteModels(kEndpoint);

  // Wait for initial model to be loaded
  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = service_->GetModels();
    return std::any_of(models.begin(), models.end(),
                       [](const auto& m) { return m->key == "initial-model"; });
  }));

  // Verify initial model loaded
  {
    const auto& models = service_->GetModels();
    bool found_initial = false;
    for (const auto& model : models) {
      if (model->key == "initial-model") {
        found_initial = true;
        break;
      }
    }
    EXPECT_TRUE(found_initial);
  }

  // Set up updated response
  test_url_loader_factory_.ClearResponses();
  SimulateSuccessfulFetch(kEndpoint, kUpdatedModelsJSON);

  // Expect observer notification when models refresh
  EXPECT_CALL(*observer_, OnModelListUpdated()).Times(testing::AtLeast(1));

  // Trigger refresh
  service_->RefreshRemoteModels();

  // Wait for updated model to be loaded
  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = service_->GetModels();
    return std::any_of(models.begin(), models.end(),
                       [](const auto& m) { return m->key == "updated-model"; });
  }));

  // Verify updated model loaded
  {
    const auto& models = service_->GetModels();
    bool found_updated = false;
    bool found_initial = false;
    for (const auto& model : models) {
      if (model->key == "updated-model") {
        found_updated = true;
      }
      if (model->key == "initial-model") {
        found_initial = true;
      }
    }
    EXPECT_TRUE(found_updated);
    EXPECT_FALSE(found_initial);
  }
}

TEST_F(ModelServiceRemoteModelsTest, IncludesCustomModelsWithRemoteModels) {
  const char kEndpoint[] = "https://example.com/models";
  const char kRemoteModelsJSON[] = R"({
    "models": [{
      "key": "remote-model",
      "display_name": "Remote Model",
      "vision_support": true,
      "supports_tools": false,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "remote-model-api",
        "category": "chat",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }]
  })";

  SimulateSuccessfulFetch(kEndpoint, kRemoteModelsJSON);
  CreateServiceWithRemoteModels(kEndpoint);

  // Wait for remote model to be loaded
  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = service_->GetModels();
    return std::any_of(models.begin(), models.end(),
                       [](const auto& m) { return m->key == "remote-model"; });
  }));

  // Add a custom model
  mojom::ModelPtr custom_model = mojom::Model::New();
  custom_model->display_name = "My Custom Model";
  custom_model->options =
      mojom::ModelOptions::NewCustomModelOptions(mojom::CustomModelOptions::New(
          "custom-model", 0, 0, 0, "", GURL("http://example.com"), ""));
  service_->AddCustomModel(std::move(custom_model));

  // Should have both remote and custom models
  const auto& models = service_->GetModels();

  bool found_remote = false;
  bool found_custom = false;
  for (const auto& model : models) {
    if (model->key == "remote-model") {
      found_remote = true;
    }
    if (model->key.find("custom:") == 0) {
      found_custom = true;
    }
  }

  EXPECT_TRUE(found_remote);
  EXPECT_TRUE(found_custom);
}

TEST_F(ModelServiceRemoteModelsTest, BackgroundRefreshWhenCacheStaleOnStartup) {
  const char kEndpoint[] = "https://example.com/models";
  const char kFreshModelsJSON[] = R"({
    "models": [{
      "key": "fresh-model",
      "display_name": "Fresh Model",
      "vision_support": false,
      "supports_tools": true,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "fresh-model-api",
        "category": "chat",
        "access": "premium",
        "max_associated_content_length": 150000,
        "long_conversation_warning_character_limit": 300000
      }
    }]
  })";

  // Populate prefs with stale cache (62 minutes old)
  base::DictValue cache;
  base::ListValue models_list;

  base::DictValue model1;
  model1.Set("key", "stale-model");
  model1.Set("display_name", "Stale Model");
  model1.Set("vision_support", true);
  model1.Set("supports_tools", false);
  model1.Set("is_suggested_model", true);
  model1.Set("is_near_model", false);

  base::DictValue options1;
  options1.Set("type", "leo");
  options1.Set("name", "stale-model-api");
  options1.Set("display_maker", "Stale Provider");
  options1.Set("category", "chat");
  options1.Set("access", "basic");
  options1.Set("max_associated_content_length", 100000);
  options1.Set("long_conversation_warning_character_limit", 200000);
  model1.Set("options", std::move(options1));

  models_list.Append(std::move(model1));
  cache.Set("models", std::move(models_list));
  // Set timestamp to 62 minutes ago (past TTL)
  cache.Set("last_updated",
            (base::Time::Now() - base::Minutes(62)).InSecondsFSinceUnixEpoch());
  cache.Set("endpoint_url", kEndpoint);

  pref_service_.SetDict(prefs::kRemoteModelsCache, std::move(cache));

  // Set up fresh response for background refresh
  SimulateSuccessfulFetch(kEndpoint, kFreshModelsJSON);

  // Expect observer notification when background refresh completes
  EXPECT_CALL(*observer_, OnModelListUpdated()).Times(testing::AtLeast(1));

  CreateServiceWithRemoteModels(kEndpoint);

  // Initially should use stale cache
  {
    const auto& models = service_->GetModels();
    bool found_stale = false;
    for (const auto& model : models) {
      if (model->key == "stale-model") {
        found_stale = true;
        break;
      }
    }
    EXPECT_TRUE(found_stale);
  }

  // Wait for background refresh to complete with fresh models
  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = service_->GetModels();
    return std::any_of(models.begin(), models.end(),
                       [](const auto& m) { return m->key == "fresh-model"; });
  }));

  // After refresh, should have fresh models
  {
    const auto& models = service_->GetModels();
    bool found_fresh = false;
    bool found_stale = false;
    for (const auto& model : models) {
      if (model->key == "fresh-model") {
        found_fresh = true;
      }
      if (model->key == "stale-model") {
        found_stale = true;
      }
    }
    EXPECT_TRUE(found_fresh);
    EXPECT_FALSE(found_stale);
  }
}

}  // namespace ai_chat
