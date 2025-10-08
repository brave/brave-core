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
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kOllamaModelsResponse[] = R"({
  "models": [
    {
      "name": "llama2:7b",
      "modified_at": "2024-01-01T00:00:00Z",
      "size": 3825819519,
      "details": {
        "parameter_size": "7B"
      }
    },
    {
      "name": "mistral:latest",
      "modified_at": "2024-01-02T00:00:00Z",
      "size": 4109865159,
      "details": {
        "parameter_size": "7B"
      }
    }
  ]
})";

constexpr char kOllamaModelsResponseUpdated[] = R"({
  "models": [
    {
      "name": "llama2:7b",
      "modified_at": "2024-01-01T00:00:00Z",
      "size": 3825819519,
      "details": {
        "parameter_size": "7B"
      }
    }
  ]
})";

constexpr char kModelDetailsResponse[] = R"({
  "model_info": {
    "general.architecture": "llama",
    "llama.context_length": 4096
  },
  "capabilities": ["chat"]
})";

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

 private:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<ModelService> model_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<OllamaModelFetcher> ollama_model_fetcher_;
};

TEST_F(OllamaModelFetcherTest, FetchModelsAddsNewModels) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponse);
  // Mock the detail responses for each model
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);

  size_t initial_count = model_service()->GetModels().size();

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
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
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
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponseUpdated);
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

TEST_F(OllamaModelFetcherTest, RemoveModelsRemovesAllOllamaModels) {
  // First add some models
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
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

  // Now remove them
  ollama_model_fetcher()->RemoveModels();

  const auto& models_after = model_service()->GetModels();
  size_t ollama_count_after = 0;
  for (const auto& model : models_after) {
    if (model->options && model->options->is_custom_model_options() &&
        model->options->get_custom_model_options()->endpoint.spec() ==
            ai_chat::mojom::kOllamaEndpoint) {
      ollama_count_after++;
    }
  }
  EXPECT_EQ(0u, ollama_count_after);
}

TEST_F(OllamaModelFetcherTest, FetchModelsHandlesEmptyResponse) {
  base::RunLoop run_loop;
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         "");

  size_t initial_count = model_service()->GetModels().size();

  ollama_model_fetcher()->FetchModels();

  // Post a task to quit after network request completes
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  const auto& models_after = model_service()->GetModels();
  EXPECT_EQ(initial_count, models_after.size());
}

TEST_F(OllamaModelFetcherTest, FetchModelsHandlesInvalidJSON) {
  base::RunLoop run_loop;
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         "invalid json");

  size_t initial_count = model_service()->GetModels().size();

  ollama_model_fetcher()->FetchModels();

  // Post a task to quit after network request completes
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  const auto& models_after = model_service()->GetModels();
  EXPECT_EQ(initial_count, models_after.size());
}

TEST_F(OllamaModelFetcherTest, PrefChangeTriggersModelFetch) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);

  size_t initial_count = model_service()->GetModels().size();

  // Enable Ollama fetching
  pref_service()->SetBoolean(prefs::kBraveAIChatOllamaFetchEnabled, true);

  EXPECT_TRUE(base::test::RunUntil([&]() {
    return model_service()->GetModels().size() == initial_count + 2;
  }));
}

TEST_F(OllamaModelFetcherTest, PrefChangeTriggersRemove) {
  // First add some models
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiShowEndpoint,
                                         kModelDetailsResponse);
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
    return count == 0;
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
