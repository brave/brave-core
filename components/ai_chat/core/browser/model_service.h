// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_

#include <stddef.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_registry_simple.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefRegistrySimple;
class PrefService;

namespace os_crypt_async {
class OSCryptAsync;
}  // namespace os_crypt_async

namespace ai_chat {
class EngineConsumer;
class AIChatCredentialManager;
class RemoteModelsProvider;

// Owns the AI chat model catalog: built-in Leo models (optionally replaced
// via a remote config fetch, see `OnRemoteModelsReady()`) and user-defined
// custom models.
//
// `ModelService` loads its model list synchronously in the constructor, so
// `GetModels()` / `GetModel()` / `GetCustomModels()` are usable immediately.
// However, the `Encryptor` for custom-model API keys arrives asynchronously
// via `OSCryptAsync::GetInstance()`. Until `OnEncryptorReady()` fires,
// stored custom-model API keys are marked unavailable. `OnModelListUpdated()`
// is dispatched when the encryptor arrives so observers can refresh engines.
// Unavailable keys cannot be used for requests or silently overwritten.
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

  ModelService(
      PrefService* prefs_service,
      os_crypt_async::OSCryptAsync* os_crypt_async,
      network::NetworkContextGetter network_context_getter,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      base::FilePath profile_path);
  ~ModelService() override;

  ModelService(const ModelService&) = delete;
  ModelService& operator=(const ModelService&) = delete;

  // KeyedService:
  void Shutdown() override;

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

  mojom::OperationResult AddCustomModel(mojom::ModelPtr model);
  mojom::OperationResult SaveCustomModel(uint32_t index, mojom::ModelPtr model);
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
  void SetEncryptorForTesting(
      scoped_refptr<os_crypt_async::Encryptor> encryptor) {
    OnEncryptorReady(std::move(encryptor));
  }
  RemoteModelsProvider* GetRemoteModelsProviderForTesting() {
    return remote_models_provider_.get();
  }
  void OnRemoteModelsReadyForTesting(std::vector<mojom::ModelPtr> models) {
    OnRemoteModelsReady(std::move(models));
  }

 private:
  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);
  void InitModels();
  // Merges a freshly fetched remote model list into `leo_models_`: every
  // existing entry except `kChatAutomaticModelKey` is dropped, then every
  // fetched entry is upserted by key. Keys present before the merge but
  // absent afterward fire `OnModelRemoved()`/`OnDefaultModelChanged()`. A
  // no-op if `fetched_models` is empty (fetch failure).
  void OnRemoteModelsReady(std::vector<mojom::ModelPtr> fetched_models);
  // Walks the custom-model prefs and updates the `api_key` on each
  // already-loaded entry in `all_models_`. Called from `OnEncryptorReady()` to
  // populate keys that were unavailable during the initial synchronous load.
  void RefreshCustomModelApiKeys();

  std::optional<std::string> EncryptAPIKey(const std::string& api_key) const;
  std::optional<std::string> DecryptAPIKey(
      const std::string& encoded_api_key) const;
  // Returns the dict-shaped representation of `model` used to persist a
  // custom model in the `kCustomModelsList` pref.
  std::optional<base::DictValue> CustomModelToPrefDict(
      mojom::ModelPtr model) const;

  base::ObserverList<Observer> observers_;
  // The full catalog returned by `GetModels()`: `leo_models_` plus custom
  // models, rebuilt from both on every `InitModels()` call.
  std::vector<ai_chat::mojom::ModelPtr> all_models_;
  // Built-in Leo models, replaced wholesale by `OnRemoteModelsReady()` when
  // remote config is enabled. Kept separate from `all_models_` so the merge
  // logic never has to distinguish Leo entries from custom ones.
  std::vector<ai_chat::mojom::ModelPtr> leo_models_;
  raw_ptr<PrefService> pref_service_;
  network::NetworkContextGetter network_context_getter_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  std::unique_ptr<RemoteModelsProvider> remote_models_provider_;

  base::WeakPtrFactory<ModelService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
