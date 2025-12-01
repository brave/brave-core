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
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

using ::testing::_;

// Mock Delegate for testing OllamaModelFetcher
class MockDelegate : public OllamaModelFetcher::Delegate {
 public:
  MockDelegate() = default;
  ~MockDelegate() override = default;

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

    // Create mock delegate
    mock_delegate_ = std::make_unique<MockDelegate>();

    // Pass mock delegate to OllamaModelFetcher
    ollama_model_fetcher_ = std::make_unique<OllamaModelFetcher>(
        *model_service_, &pref_service_, mock_delegate_.get());
  }

  void TearDown() override { OSCryptMocker::TearDown(); }

  ModelService* model_service() { return model_service_.get(); }
  OllamaModelFetcher* ollama_model_fetcher() {
    return ollama_model_fetcher_.get();
  }
  sync_preferences::TestingPrefServiceSyncable* pref_service() {
    return &pref_service_;
  }
  MockDelegate* mock_delegate() { return mock_delegate_.get(); }

 private:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<MockDelegate> mock_delegate_;
  std::unique_ptr<OllamaModelFetcher> ollama_model_fetcher_;
};

TEST_F(OllamaModelFetcherTest, FetchModelsAddsNewModels) {
  size_t initial_count = model_service()->GetModels().size();

  // Setup mock expectations
  std::vector<std::string> mock_models = {"llama2:7b", "mistral:latest"};
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaModelFetcher::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_delegate(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_delegate(), ShowModel("mistral:latest", _))
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
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaModelFetcher::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_delegate(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_delegate(), ShowModel("mistral:latest", _))
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
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
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
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
      .WillOnce([&callback_called](
                    OllamaModelFetcher::Delegate::ModelsCallback callback) {
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
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
      .WillOnce([&callback_called](
                    OllamaModelFetcher::Delegate::ModelsCallback callback) {
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
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaModelFetcher::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_delegate(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_delegate(), ShowModel("mistral:latest", _))
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
  EXPECT_CALL(*mock_delegate(), FetchModels(_))
      .WillOnce(base::test::RunOnceCallback<0>(std::move(mock_models)));

  OllamaModelFetcher::ModelDetails details;
  details.context_length = 4096;
  details.has_vision = false;

  EXPECT_CALL(*mock_delegate(), ShowModel("llama2:7b", _))
      .WillOnce(base::test::RunOnceCallback<1>(details));
  EXPECT_CALL(*mock_delegate(), ShowModel("mistral:latest", _))
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

  // Disable Ollama fetching
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

// Tests for FormatOllamaModelName function

TEST(OllamaModelNameFormattingTest, FormatOllamaModelName_RemovesLatestSuffix) {
  // :latest suffix is removed, numbers separated from letters
  EXPECT_EQ("Llama 2",
            OllamaModelFetcher::FormatOllamaModelName("llama2:latest"));
  EXPECT_EQ("Mistral",
            OllamaModelFetcher::FormatOllamaModelName("mistral:latest"));
}

TEST(OllamaModelNameFormattingTest,
     FormatOllamaModelName_ReplacesColonsAndHyphensWithSpaces) {
  // Colons/hyphens become spaces, single letter sizes stay attached
  EXPECT_EQ("Llama 2 7B",
            OllamaModelFetcher::FormatOllamaModelName("llama2:7b"));
  EXPECT_EQ("Code Llama 13B",
            OllamaModelFetcher::FormatOllamaModelName("code-llama-13b"));
}

TEST(OllamaModelNameFormattingTest, FormatOllamaModelName_CapitalizesWords) {
  // Numbers separated from letters, each word capitalized
  EXPECT_EQ("Llama 2", OllamaModelFetcher::FormatOllamaModelName("llama2"));
  EXPECT_EQ("Mistral", OllamaModelFetcher::FormatOllamaModelName("mistral"));
  EXPECT_EQ("Code Llama",
            OllamaModelFetcher::FormatOllamaModelName("code-llama"));
}

TEST(OllamaModelNameFormattingTest,
     FormatOllamaModelName_PreservesSingleLetterVersions) {
  // Single letter 'v' before number is kept with number
  EXPECT_EQ("Llama V1.6",
            OllamaModelFetcher::FormatOllamaModelName("llama-v1.6"));
  EXPECT_EQ("Mistral V2",
            OllamaModelFetcher::FormatOllamaModelName("mistral:v2"));
}

TEST(OllamaModelNameFormattingTest,
     FormatOllamaModelName_PreservesParameterSizes) {
  // Numbers separated, but single letter sizes (7b, 13b) stay attached
  EXPECT_EQ("Llama 2 7B",
            OllamaModelFetcher::FormatOllamaModelName("llama2-7b"));
  EXPECT_EQ("Mistral 13B",
            OllamaModelFetcher::FormatOllamaModelName("mistral:13b"));
  EXPECT_EQ("Codellama 34B",
            OllamaModelFetcher::FormatOllamaModelName("codellama-34b"));
}

TEST(OllamaModelNameFormattingTest,
     FormatOllamaModelName_HandlesMultipleWords) {
  EXPECT_EQ("Neural Chat 7B",
            OllamaModelFetcher::FormatOllamaModelName("neural-chat-7b"));
  EXPECT_EQ("Stable Beluga 13B",
            OllamaModelFetcher::FormatOllamaModelName("stable-beluga-13b"));
}

TEST(OllamaModelNameFormattingTest, FormatOllamaModelName_TrimsSpaces) {
  // Input with spaces get trimmed by our function
  EXPECT_EQ("Llama 2",
            OllamaModelFetcher::FormatOllamaModelName(" llama2    "));
  EXPECT_EQ("Mistral",
            OllamaModelFetcher::FormatOllamaModelName(" mistral:latest    "));
}

TEST(OllamaModelNameFormattingTest, FormatOllamaModelName_HandlesEmptyString) {
  EXPECT_EQ("", OllamaModelFetcher::FormatOllamaModelName(""));
}

TEST(OllamaModelNameFormattingTest, FormatOllamaModelName_HandlesComplexNames) {
  // Single letter 'v' before number stays lowercase as a version indicator
  EXPECT_EQ(
      "Deepseek Coder V1.5 16B",
      OllamaModelFetcher::FormatOllamaModelName("deepseek-coder:v1.5-16b"));
  EXPECT_EQ("Llava V1.6 34B",
            OllamaModelFetcher::FormatOllamaModelName("llava:v1.6-34b"));
}

TEST(OllamaModelNameFormattingTest,
     FormatOllamaModelName_HandlesNumbersInMiddle) {
  // Numbers in the middle cause spacing
  EXPECT_EQ("Gpt 4 All", OllamaModelFetcher::FormatOllamaModelName("gpt4-all"));
  EXPECT_EQ("Falcon 180B",
            OllamaModelFetcher::FormatOllamaModelName("falcon-180b"));
}

TEST(OllamaModelNameFormattingTest,
     FormatOllamaModelName_PreservesOriginalOnAllSpaces) {
  EXPECT_EQ("   ", OllamaModelFetcher::FormatOllamaModelName("   "));
}

}  // namespace ai_chat
