/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

class ModelService;

// Manages fetching of models from Ollama to the AI Chat
// ModelService.
class OllamaModelFetcher {
 public:
  OllamaModelFetcher(
      ModelService& model_service,
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OllamaModelFetcher();

  OllamaModelFetcher(const OllamaModelFetcher&) = delete;
  OllamaModelFetcher& operator=(const OllamaModelFetcher&) = delete;

 private:
  friend class OllamaModelFetcherTest;
  FRIEND_TEST_ALL_PREFIXES(OllamaModelFetcherTest, FetchModelsAddsNewModels);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelFetcherTest,
                           FetchModelsRemovesObsoleteModels);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelFetcherTest,
                           FetchModelsHandlesEmptyResponse);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelFetcherTest,
                           FetchModelsHandlesInvalidJSON);

  void OnOllamaFetchEnabledChanged();
  void FetchModels();
  void OnModelsFetched(std::optional<std::vector<std::string>> models);
  struct PendingModelInfo {
    std::string model_name;
    std::string display_name;
  };

  void FetchModelDetails(const std::string& model_name);
  void OnModelDetailsFetched(
      const std::string& model_name,
      std::optional<OllamaService::ModelDetails> details);

  const raw_ref<ModelService> model_service_;
  raw_ptr<PrefService> prefs_;
  std::unique_ptr<OllamaService> ollama_service_;
  PrefChangeRegistrar pref_change_registrar_;
  std::map<std::string, PendingModelInfo> pending_models_;
  base::WeakPtrFactory<OllamaModelFetcher> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_
