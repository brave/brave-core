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
#include "components/prefs/pref_change_registrar.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

class ModelService;
class OllamaClient;

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

  // Trigger a fetch of Ollama models
  void FetchModels();

  // Remove all Ollama models from ModelService
  void RemoveModels();

 private:
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_RemovesLatestSuffix);
  FRIEND_TEST_ALL_PREFIXES(
      OllamaModelNameFormattingTest,
      FormatOllamaModelName_ReplacesColonsAndHyphensWithSpaces);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_CapitalizesWords);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_PreservesSingleLetterVersions);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_PreservesParameterSizes);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_HandlesMultipleWords);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_TrimsSpaces);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_HandlesEmptyString);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_HandlesComplexNames);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_HandlesNumbersInMiddle);
  FRIEND_TEST_ALL_PREFIXES(OllamaModelNameFormattingTest,
                           FormatOllamaModelName_PreservesOriginalOnAllSpaces);

  // Formats Ollama model names for display in the UI.
  // Examples:
  //   "llama2:7b" -> "Llama 2 7B"
  //   "mistral:latest" -> "Mistral"
  //   "codellama-13b" -> "Codellama 13B"
  static std::string FormatOllamaModelName(const std::string& raw_name);

  void OnOllamaFetchEnabledChanged();
  void OnModelsResponse(std::optional<std::string> response_body);
  void ProcessModelsResponse(const std::string& response_body);
  struct PendingModelInfo {
    std::string model_name;
    std::string display_name;
  };

  void FetchModelDetails(const std::string& model_name);
  void OnModelDetailsResponse(const std::string& model_name,
                              std::optional<std::string> response_body);

  const raw_ref<ModelService> model_service_;
  raw_ptr<PrefService> prefs_;
  std::unique_ptr<OllamaClient> ollama_client_;
  PrefChangeRegistrar pref_change_registrar_;
  std::map<std::string, PendingModelInfo> pending_models_;
  base::WeakPtrFactory<OllamaModelFetcher> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_MODEL_FETCHER_H_
