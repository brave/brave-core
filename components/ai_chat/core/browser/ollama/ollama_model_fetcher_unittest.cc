/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_model_fetcher.h"

#include <memory>
#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

using ::testing::_;

// Mock OllamaService for testing OllamaModelFetcher
class MockOllamaService : public OllamaService {
 public:
  explicit MockOllamaService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : OllamaService(url_loader_factory) {}

  MOCK_METHOD(void, FetchModels, (ModelsCallback), (override));
  MOCK_METHOD(void,
              ShowModel,
              (const std::string&, ModelDetailsCallback),
              (override));
};

}  // namespace

class OllamaModelFetcherTest : public testing::Test {
 public:
  OllamaModelFetcherTest() = default;
  ~OllamaModelFetcherTest() override = default;

  void SetUp() override {
    OSCryptMocker::SetUp();
    prefs::RegisterProfilePrefs(pref_service_.registry());
    ModelService::RegisterProfilePrefs(pref_service_.registry());

    model_service_ = std::make_unique<ModelService>(&pref_service_);

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);

    ollama_model_fetcher_ = std::make_unique<OllamaModelFetcher>(
        *model_service_, &pref_service_, shared_url_loader_factory_);

    // Inject mock OllamaService for testing
    auto mock = std::make_unique<MockOllamaService>(shared_url_loader_factory_);
    mock_ollama_service_ = mock.get();
    ollama_model_fetcher_->ollama_service_ = std::move(mock);
  }

  void TearDown() override { OSCryptMocker::TearDown(); }

  ModelService* model_service() { return model_service_.get(); }
  OllamaModelFetcher* ollama_model_fetcher() {
    return ollama_model_fetcher_.get();
  }
  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &test_url_loader_factory_;
  }
  sync_preferences::TestingPrefServiceSyncable* pref_service() {
    return &pref_service_;
  }
  MockOllamaService* mock_ollama_service() { return mock_ollama_service_; }

 private:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<ModelService> model_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<OllamaModelFetcher> ollama_model_fetcher_;
  raw_ptr<MockOllamaService> mock_ollama_service_;
};

TEST_F(OllamaModelFetcherTest, FetchModelsAddsNewModels) {
  size_t initial_count = model_service()->GetModels().size();

  // Setup mock expectations
  std::vector<std::string> mock_models = {"llama2:7b", "mistral:latest"};
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaService::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_ollama_service(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_ollama_service(), ShowModel("mistral:latest", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));

  // Trigger FetchModels
  ollama_model_fetcher()->FetchModels();

  EXPECT_TRUE(base::test::RunUntil([&]() {
    return model_service()->GetModels().size() == initial_count + 2;
  }));

  // Check that Ollama models were added
  const auto& models_after = model_service()->GetModels();
  int ollama_count = 0;
  for (const auto& model : models_after) {
    if (model->options && model->options->is_custom_model_options() &&
        model->options->get_custom_model_options()->endpoint.spec() ==
            ai_chat::mojom::kOllamaEndpoint) {
      ollama_count++;
    }
  }
  EXPECT_EQ(2, ollama_count);
}

TEST_F(OllamaModelFetcherTest, FetchModelsRemovesObsoleteModels) {
  // First fetch - add 2 models
  std::vector<std::string> mock_models = {"llama2:7b", "mistral:latest"};
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaService::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_ollama_service(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_ollama_service(), ShowModel("mistral:latest", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));

  ollama_model_fetcher()->FetchModels();

  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = model_service()->GetModels();
    size_t count = 0;
    for (const auto& model : models) {
      if (model->options && model->options->is_custom_model_options() &&
          model->options->get_custom_model_options()->endpoint.spec() ==
              ai_chat::mojom::kOllamaEndpoint) {
        count++;
      }
    }
    return count == 2;
  }));

  // Second fetch - only 1 model remains (llama2 is not new, so no detail fetch
  // needed)
  std::vector<std::string> updated_models = {"llama2:7b"};
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(updated_models)));

  ollama_model_fetcher()->FetchModels();

  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = model_service()->GetModels();
    size_t count = 0;
    for (const auto& model : models) {
      if (model->options && model->options->is_custom_model_options() &&
          model->options->get_custom_model_options()->endpoint.spec() ==
              ai_chat::mojom::kOllamaEndpoint) {
        count++;
      }
    }
    return count == 1;
  }));
}

TEST_F(OllamaModelFetcherTest, FetchModelsHandlesEmptyResponse) {
  size_t initial_count = model_service()->GetModels().size();

  bool callback_called = false;
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce([&callback_called](OllamaService::ModelsCallback callback) {
        std::move(callback).Run(std::nullopt);
        callback_called = true;
      });

  ollama_model_fetcher()->FetchModels();

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));

  const auto& models_after = model_service()->GetModels();
  EXPECT_EQ(initial_count, models_after.size());
}

TEST_F(OllamaModelFetcherTest, FetchModelsHandlesInvalidJSON) {
  size_t initial_count = model_service()->GetModels().size();

  bool callback_called = false;
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce([&callback_called](OllamaService::ModelsCallback callback) {
        std::move(callback).Run(std::nullopt);
        callback_called = true;
      });

  ollama_model_fetcher()->FetchModels();

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));

  const auto& models_after = model_service()->GetModels();
  EXPECT_EQ(initial_count, models_after.size());
}

TEST_F(OllamaModelFetcherTest, PrefChangeTriggersModelFetch) {
  size_t initial_count = model_service()->GetModels().size();

  std::vector<std::string> mock_models = {"llama2:7b", "mistral:latest"};
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaService::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_ollama_service(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_ollama_service(), ShowModel("mistral:latest", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));

  // Enable Ollama fetching - this will trigger FetchModels
  pref_service()->SetBoolean(prefs::kBraveAIChatOllamaFetchEnabled, true);

  EXPECT_TRUE(base::test::RunUntil([&]() {
    return model_service()->GetModels().size() == initial_count + 2;
  }));
}

TEST_F(OllamaModelFetcherTest, PrefChangeDoesntTriggersRemove) {
  // First add some models
  std::vector<std::string> mock_models = {"llama2:7b", "mistral:latest"};
  EXPECT_CALL(*mock_ollama_service(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaService::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_ollama_service(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_ollama_service(), ShowModel("mistral:latest", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));

  pref_service()->SetBoolean(prefs::kBraveAIChatOllamaFetchEnabled, true);

  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = model_service()->GetModels();
    size_t count = 0;
    for (const auto& model : models) {
      if (model->options && model->options->is_custom_model_options() &&
          model->options->get_custom_model_options()->endpoint.spec() ==
              ai_chat::mojom::kOllamaEndpoint) {
        count++;
      }
    }
    return count == 2;
  }));

  // Disable Ollama fetching - this triggers RemoveModels
  pref_service()->SetBoolean(prefs::kBraveAIChatOllamaFetchEnabled, false);

  EXPECT_TRUE(base::test::RunUntil([&]() {
    const auto& models = model_service()->GetModels();
    size_t count = 0;
    for (const auto& model : models) {
      if (model->options && model->options->is_custom_model_options() &&
          model->options->get_custom_model_options()->endpoint.spec() ==
              ai_chat::mojom::kOllamaEndpoint) {
        count++;
      }
    }
    return count == 2;
  }));
}

}  // namespace ai_chat
