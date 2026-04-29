// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_service.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/metrics/field_trial_params.h"
#include "base/numerics/safe_math.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
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
  MOCK_METHOD(void, OnModelListUpdated, (), (override));
  MOCK_METHOD(void, OnModelRemoved, (const std::string&), (override));

 private:
  base::ScopedObservation<ModelService, ModelService::Observer>
      models_observer_{this};
};

// RAII observer that quits its `RunLoop` once `ModelService` finishes loading
// the model list. Used to wait for the async `OSCryptAsync`-driven encryptor
// arrival.
class ScopedModelListReadyObserver : public ModelService::Observer {
 public:
  ScopedModelListReadyObserver(ModelService& service, base::OnceClosure quit)
      : service_(service), quit_(std::move(quit)) {
    service_->AddObserver(this);
  }
  ~ScopedModelListReadyObserver() override { service_->RemoveObserver(this); }

  void OnModelListUpdated() override {
    if (quit_) {
      std::move(quit_).Run();
    }
  }

 private:
  const raw_ref<ModelService> service_;
  base::OnceClosure quit_;
};

}  // namespace

class ModelServiceTest : public ::testing::Test {
 public:
  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    prefs::RegisterProfilePrefsForMigration(pref_service_.registry());
    ModelService::RegisterProfilePrefs(pref_service_.registry());
    observer_ = std::make_unique<NiceMock<MockModelServiceObserver>>();
  }

  ModelService* GetService() {
    if (!service_) {
      service_ =
          std::make_unique<ModelService>(&pref_service_, os_crypt_async_.get());
      observer_->Observe(service_.get());
    }
    return service_.get();
  }

  void TearDown() override { observer_.reset(); }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_ =
      os_crypt_async::GetTestOSCryptAsyncForTesting(
          /*is_sync_for_unittests=*/true);
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

// Fixture that exercises `ModelService` against an asynchronous test
// `OSCryptAsync` (default `is_sync_for_unittests=false`), so the encryptor's
// arrival is a real posted task observable from tests.
class ModelServiceAsyncEncryptorTest : public ::testing::Test {
 public:
  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    prefs::RegisterProfilePrefsForMigration(pref_service_.registry());
    ModelService::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  // Preloads the `kCustomModelsList` pref with custom models so the test
  // subject reads them at construction. Uses a temporary `ModelService` on
  // the same `OSCryptAsync` to write the entries (encryption requires the
  // encryptor). Each input is `(model_request_name, api_key)`.
  void PreloadCustomModelsInPrefs(
      const std::vector<std::pair<std::string, std::string>>& models) {
    auto service =
        std::make_unique<ModelService>(&pref_service_, os_crypt_async_.get());
    // Wait for `OnModelListUpdated()` so the encryptor is ready before we
    // call `AddCustomModel()` — otherwise `EncryptAPIKey()` would persist
    // empty strings and the test subject would never see the real keys.
    base::RunLoop run_loop;
    ScopedModelListReadyObserver observer(*service, run_loop.QuitClosure());
    run_loop.Run();

    for (const auto& [request_name, api_key] : models) {
      auto model = mojom::Model::New();
      model->display_name = request_name;
      model->options = mojom::ModelOptions::NewCustomModelOptions(
          mojom::CustomModelOptions::New(request_name, 0, 0, 0, std::nullopt,
                                         GURL("https://test.example.com/api"),
                                         api_key));
      service->AddCustomModel(std::move(model));
    }
  }

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_ =
      os_crypt_async::GetTestOSCryptAsyncForTesting(
          /*is_sync_for_unittests=*/false);

 private:
  base::test::TaskEnvironment task_environment_;
};

// Verifies that custom models loaded synchronously at construction have
// empty `api_key`s until the asynchronous `Encryptor` arrives, after which
// `RefreshCustomModelApiKeys()` updates every entry in-place and
// `OnModelListUpdated()` fires. Includes a model with an empty `api_key` to
// cover the no-op refresh path.
TEST_F(ModelServiceAsyncEncryptorTest,
       CustomModelApiKeyRefreshedAfterEncryptor) {
  PreloadCustomModelsInPrefs({
      {"model-alpha", "key-alpha"},
      {"model-beta", std::string()},  // empty key — should remain empty
      {"model-gamma", "key-gamma"},
  });

  auto service =
      std::make_unique<ModelService>(&pref_service_, os_crypt_async_.get());

  // Pre-encryptor: all three are present with empty api_keys.
  auto before = service->GetCustomModels();
  ASSERT_EQ(before.size(), 3u);
  for (const auto& model : before) {
    ASSERT_TRUE(model->options->is_custom_model_options());
    EXPECT_EQ(model->options->get_custom_model_options()->api_key,
              std::string());
  }

  // Wait for the in-place refresh.
  base::RunLoop run_loop;
  ScopedModelListReadyObserver observer(*service, run_loop.QuitClosure());
  run_loop.Run();

  // Post-encryptor: each model's api_key matches expectation. Order matches
  // insertion order in both prefs and `models_`.
  auto after = service->GetCustomModels();
  ASSERT_EQ(after.size(), 3u);
  ASSERT_TRUE(after[0]->options->is_custom_model_options());
  EXPECT_EQ(after[0]->options->get_custom_model_options()->model_request_name,
            "model-alpha");
  EXPECT_EQ(after[0]->options->get_custom_model_options()->api_key,
            "key-alpha");
  ASSERT_TRUE(after[1]->options->is_custom_model_options());
  EXPECT_EQ(after[1]->options->get_custom_model_options()->model_request_name,
            "model-beta");
  EXPECT_TRUE(after[1]->options->get_custom_model_options()->api_key.empty());
  ASSERT_TRUE(after[2]->options->is_custom_model_options());
  EXPECT_EQ(after[2]->options->get_custom_model_options()->model_request_name,
            "model-gamma");
  EXPECT_EQ(after[2]->options->get_custom_model_options()->api_key,
            "key-gamma");
}

}  // namespace ai_chat
