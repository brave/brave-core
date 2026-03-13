// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_

#include <stddef.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_registry_simple.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace ai_chat {
class EngineConsumer;
class AIChatCredentialManager;
class RemoteModelsFetcher;

class ModelService : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    // Returns removed model key.
    virtual void OnModelRemoved(const std::string& removed_key) {}
    virtual void OnModelListUpdated() {}
    virtual void OnDefaultModelChanged(const std::string& old_key,
                                       const std::string& new_key) {}
  };

  explicit ModelService(PrefService* profile_prefs);
  ModelService(
      PrefService* profile_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~ModelService() override;

  ModelService(const ModelService&) = delete;
  ModelService& operator=(const ModelService&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void MigrateProfilePrefs(PrefService* profile_prefs);
  static void SetAssociatedContentLengthMetrics(mojom::Model& model);
  static size_t CalcuateMaxAssociatedContentLengthForModel(
      const mojom::Model& model);

  void OnPremiumStatus(mojom::PremiumStatus status);

  // All models that the user can choose for chat conversations, in UI display
  // order.
  const std::vector<ai_chat::mojom::ModelPtr>& GetModels();
  std::vector<ai_chat::mojom::ModelWithSubtitlePtr> GetModelsWithSubtitles();
  const ai_chat::mojom::Model* GetModel(std::string_view key);

  std::optional<std::string> GetLeoModelKeyByName(std::string_view name);
  std::optional<std::string> GetLeoModelNameByKey(std::string_view key);

  void AddCustomModel(mojom::ModelPtr model);
  void SaveCustomModel(uint32_t index, mojom::ModelPtr model);
  void DeleteCustomModel(uint32_t index);

  // Delete custom models matching a predicate
  using CustomModelPredicate =
      base::RepeatingCallback<bool(const base::DictValue&)>;
  void MaybeDeleteCustomModels(CustomModelPredicate predicate);

  // Get all custom models
  const std::vector<ai_chat::mojom::ModelPtr> GetCustomModels();
  void SetDefaultModelKey(const std::string& model_key);
  const std::string& GetDefaultModelKey();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Refresh remote models from endpoint (manual cache invalidation).
  void RefreshRemoteModels();

  // Validate if a model key exists in the current model list.
  bool IsValidModelKey(const std::string& model_key) const;

  // Get a fallback model key when the requested model is unavailable.
  // Priority: Automatic model > first suggested model > first available model.
  std::string GetFallbackModelKey() const;

  // TODO(petemill): not ideal to take these params that engine's happen
  // to need. Perhaps put this function on AIChatService, which will
  // likely directly have access to any params any engine needs.
  std::unique_ptr<EngineConsumer> GetEngineForModel(
      std::string model_key,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager);

  // Returns a static model
  static const mojom::Model* GetModelForTesting(std::string_view key);
  void SetDefaultModelKeyWithoutValidationForTesting(
      const std::string& model_key);

 private:
  enum class RemoteModelsState {
    DISABLED,
    FETCHING_INITIAL,  // Initial fetch in progress, no cache
    FETCHING_REFRESH,  // Refresh fetch in progress, have cache
    CACHED_FRESH,
    CACHED_STALE,
    FAILED_NO_CACHE  // Fetch failed and no cache available
  };

  void InitializeCommon();
  void InitModels();

  std::vector<mojom::ModelPtr> GetModelsForInitialization();

  // Used when feature is disabled.
  std::vector<mojom::ModelPtr> GetStaticLeoModels();

  // Used during initial fetch or on fetch failure with no cache.
  std::vector<mojom::ModelPtr> GetAutomaticModelOnly();

  // Initiate remote models fetch.
  void InitRemoteModels();

  // Handle remote models fetch completion.
  void OnRemoteModelsFetched(std::vector<mojom::ModelPtr> models);

  base::ObserverList<Observer> observers_;
  std::vector<ai_chat::mojom::ModelPtr> models_;
  raw_ptr<PrefService> pref_service_;
  bool is_migrating_claude_instant_ = false;

  // Remote models support
  std::unique_ptr<RemoteModelsFetcher> remote_models_fetcher_;
  bool using_remote_models_ = false;
  RemoteModelsState remote_models_state_ = RemoteModelsState::DISABLED;

  base::WeakPtrFactory<ModelService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
