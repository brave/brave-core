// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/observer_list.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

namespace ai_chat {

class ModelService : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    // Returns removed model key.
    virtual void OnModelRemoved(const std::string& removed_key) {}
    virtual void OnModelListUpdated() {}
    virtual void OnDefaultModelChanged(const std::string& new_key) {}
  };

  explicit ModelService(PrefService* profile_prefs);
  ~ModelService() override;

  ModelService(const ModelService&) = delete;
  ModelService& operator=(const ModelService&) = delete;

  // All models that the user can choose for chat conversations, in UI display
  // order.
  const std::vector<ai_chat::mojom::ModelPtr>& GetModels();
  const ai_chat::mojom::Model* GetModel(std::string_view key);

  void AddCustomModel(mojom::ModelPtr model);
  void SaveCustomModel(uint32_t index, mojom::ModelPtr model);
  void DeleteCustomModel(uint32_t index);
  void ChangeDefaultModel(const std::string& model_key);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Returns a static model
  static const mojom::Model* GetModelForTesting(std::string_view key);

 private:
  void InitModels();
  std::vector<ai_chat::mojom::ModelPtr> GetCustomModelsFromPrefs();

  base::ObserverList<Observer> observers_;
  std::vector<ai_chat::mojom::ModelPtr> models_;
  raw_ptr<PrefService> pref_service_;
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<ModelService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_SERVICE_H_
