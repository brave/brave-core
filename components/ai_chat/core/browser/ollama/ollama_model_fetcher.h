/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_change_registrar.h"

namespace ai_chat {

class ModelService;

// Manages fetching of models from Ollama to the AI Chat
// ModelService.
class OllamaModelFetcher {
 public:
  // Contains model details fetched from Ollama.
  struct ModelDetails {
    uint32_t context_length = 0;
    bool has_vision = false;
  };

  // Delegate interface for Ollama API operations.
  // This allows OllamaModelFetcher to be decoupled from OllamaService.
  class Delegate {
   public:
    using ModelsCallback =
        base::OnceCallback<void(std::optional<std::vector<std::string>>)>;
    using ModelDetailsCallback =
        base::OnceCallback<void(std::optional<ModelDetails>)>;

    virtual ~Delegate() = default;

    // Fetch available models from Ollama.
    virtual void FetchModels(ModelsCallback callback) = 0;

    // Fetch detailed information for a specific model.
    virtual void ShowModel(const std::string& model_name,
                           ModelDetailsCallback callback) = 0;
  };

  OllamaModelFetcher(ModelService& model_service,
                     PrefService* prefs,
                     Delegate* delegate);
  ~OllamaModelFetcher();

  OllamaModelFetcher(const OllamaModelFetcher&) = delete;
  OllamaModelFetcher& operator=(const OllamaModelFetcher&) = delete;

  // Set the delegate for Ollama API operations. This must be called
  // before the fetcher is used if constructed with a nullptr delegate.
  void SetDelegate(Delegate* delegate);

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
  void OnModelDetailsFetched(const std::string& model_name,
                             std::optional<ModelDetails> details);

  // TODO(https://github.com/brave/brave-browser/issues/49828): display_name is
  // currently identical to model_name but will be used for proper name
  // formatting when we reland the formatting patch
  // (https://github.com/brave/brave-core/commit/dadaf97432efd7d64289bd92b7d31cc6e16722f0)
  const raw_ref<ModelService> model_service_;
  raw_ptr<PrefService> prefs_;
  raw_ptr<Delegate> delegate_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  std::map<std::string, PendingModelInfo> pending_models_;
  base::WeakPtrFactory<OllamaModelFetcher> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_
