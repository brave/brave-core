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
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
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

// Owns the AI chat model catalog: built-in Leo models and user-defined
// custom models.
//
// `ModelService` loads its model list synchronously in the constructor, so
// `GetModels()` / `GetModel()` / `GetCustomModels()` are usable immediately.
// However, the `Encryptor` for custom-model API keys arrives asynchronously
// via `OSCryptAsync::GetInstance()`. Until `OnEncryptorReady()` fires,
// custom-model API keys decrypt to empty strings; `OnModelListUpdated()` is
// then dispatched so observers (e.g. `ConversationHandler`) can refresh
// engines via `engine_->UpdateModelOptions()`. Requests issued against a
// custom model in this pre-encryptor window goes out with an empty API key and
// the server will reject it (typically HTTP 401 / 403). Once engine refreshes,
// subsequent requests would have valid API keys.
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

  ModelService(PrefService* prefs_service,
               os_crypt_async::OSCryptAsync* os_crypt_async,
               network::NetworkContextGetter network_context_getter,
               scoped_refptr<network::SharedURLLoaderFactory>
                   url_loader_factory = nullptr,
               base::FilePath profile_path = {});
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
  RemoteModelsProvider* GetRemoteModelsProviderForTesting() {
    return remote_models_provider_.get();
  }
  void OnRemoteModelsReadyForTesting(std::vector<mojom::ModelPtr> models) {
    OnRemoteModelsReady(std::move(models));
  }
  bool IsRemoteModelsRefreshTimerRunningForTesting() const {
    return remote_models_refresh_timer_.IsRunning();
  }
  size_t GetRemoteModelsVisibleSurfaceCountForTesting() const {
    return remote_models_visible_surface_count_;
  }

  // Called by the small `WebContentsObserver` helper owned by `AIChatUI`
  // (desktop) and its iOS equivalent whenever a Leo surface's visibility
  // changes. Ref-counted rather than boolean, since a side panel in one
  // window and a full-page tab in another can be visible simultaneously. On
  // a 0->1 transition, fetches immediately and starts a self-rescheduling
  // refresh timer; on a 1->0 transition, stops it.
  void OnRemoteModelsSurfaceVisible();
  void OnRemoteModelsSurfaceHidden();

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
  // already-loaded entry in `models_`. Called from `OnEncryptorReady()` to
  // populate keys that decrypted to empty strings during initial sync load.
  void RefreshCustomModelApiKeys();

  // Requests a fresh model list via `remote_models_provider_` (cache-first).
  // A no-op if the feature is disabled. Callers must not invoke this while a
  // request is already in flight.
  void RequestRemoteModelsRefresh();
  void OnRemoteModelsRefreshComplete(
      std::vector<mojom::ModelPtr> fetched_models);
  // Starts `remote_models_refresh_timer_` with a delay computed from
  // `kRemoteModelsCachedAt`/`kRemoteModelsCacheTTL`, so the next refresh
  // lands close to actual cache expiry rather than a fixed interval from
  // surface-open time. Falls back to a full TTL interval after a failed
  // attempt, rather than retrying quickly.
  void ScheduleNextRemoteModelsRefresh(bool last_attempt_succeeded);
  void OnRemoteModelsRefreshTimerFired();

  std::string EncryptAPIKey(const std::string& api_key) const;
  std::string DecryptAPIKey(const std::string& encoded_api_key) const;
  // Returns the dict-shaped representation of `model` used to persist a
  // custom model in the `kCustomModelsList` pref.
  base::DictValue CustomModelToPrefDict(mojom::ModelPtr model) const;

  base::ObserverList<Observer> observers_;
  std::vector<ai_chat::mojom::ModelPtr> models_;
  std::vector<ai_chat::mojom::ModelPtr> leo_models_;
  raw_ptr<PrefService> pref_service_;
  network::NetworkContextGetter network_context_getter_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  std::unique_ptr<RemoteModelsProvider> remote_models_provider_;
  size_t remote_models_visible_surface_count_ = 0;
  bool remote_models_refresh_in_flight_ = false;
  base::OneShotTimer remote_models_refresh_timer_;
  bool is_migrating_claude_instant_ = false;

  base::WeakPtrFactory<ModelService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
