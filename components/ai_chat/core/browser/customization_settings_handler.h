// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CUSTOMIZATION_SETTINGS_HANDLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CUSTOMIZATION_SETTINGS_HANDLER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace ai_chat {

class CustomizationSettingsHandler
    : public mojom::CustomizationSettingsHandler {
 public:
  // Provides what the handler can't reach itself.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Finds the memories related to `query` by meaning. `callback` gets null
    // while semantic search is unavailable.
    virtual void SearchMemories(
        const std::string& query,
        base::OnceCallback<void(const std::optional<std::vector<std::string>>&)>
            callback) = 0;
  };

  // `delegate` can be null, which leaves semantic search unavailable.
  explicit CustomizationSettingsHandler(PrefService* prefs,
                                        std::unique_ptr<Delegate> delegate);
  ~CustomizationSettingsHandler() override;

  CustomizationSettingsHandler(const CustomizationSettingsHandler&) = delete;
  CustomizationSettingsHandler& operator=(const CustomizationSettingsHandler&) =
      delete;

  // mojom::CustomizationSettingsHandler:
  void BindUI(mojo::PendingRemote<mojom::CustomizationSettingsUI> ui) override;

  // customization settings
  void GetCustomizations(GetCustomizationsCallback callback) override;
  void SetCustomizations(mojom::CustomizationsPtr customizations,
                         SetCustomizationsCallback callback) override;

  // memories
  void AddMemory(const std::string& memory,
                 AddMemoryCallback callback) override;
  void EditMemory(const std::string& old_memory,
                  const std::string& new_memory,
                  EditMemoryCallback callback) override;
  void DeleteMemory(const std::string& memory) override;
  void DeleteAllMemories() override;
  void GetMemories(GetMemoriesCallback callback) override;
  void SearchMemories(const std::string& query,
                      SearchMemoriesCallback callback) override;

 private:
  // Called when customization preferences change
  void OnCustomizationsChanged();

  // Called when memory preferences change
  void OnMemoriesChanged();

  // Interface to communicate with the settings page in the renderer.
  mojo::Remote<mojom::CustomizationSettingsUI> ui_;

  std::unique_ptr<Delegate> delegate_;

  // Profile preferences service for customization and memory data persistence.
  raw_ptr<PrefService> prefs_ = nullptr;

  // Watches for changes to customization and memory-related preferences.
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<CustomizationSettingsHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CUSTOMIZATION_SETTINGS_HANDLER_H_
