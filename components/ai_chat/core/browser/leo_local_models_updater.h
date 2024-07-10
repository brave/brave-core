/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEO_LOCAL_MODELS_UPDATER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEO_LOCAL_MODELS_UPDATER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class LeoLocalModelsUpdaterTest;

namespace ai_chat {

extern const char kUniversalQAModelName[];

class LeoLocalModelsUpdater : public brave_component_updater::BraveComponent {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLeoLocalModelsReady() {}

   protected:
    ~Observer() override = default;
  };

  LeoLocalModelsUpdater(BraveComponent::Delegate* component_delegate,
                        const base::FilePath& user_data_dir);
  LeoLocalModelsUpdater(const LeoLocalModelsUpdater&) = delete;
  LeoLocalModelsUpdater& operator=(const LeoLocalModelsUpdater&) = delete;
  ~LeoLocalModelsUpdater() override;

  void Register();
  void Cleanup(base::OnceCallback<void(bool)> reply_callback = {});

  const base::FilePath& GetUniversalQAModel() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::LeoLocalModelsUpdaterTest;

  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  static void SetUserDataDirForTest(const base::FilePath& user_data_dir);
  static std::string g_component_id_;
  static std::string g_component_base64_public_key_;
  static base::FilePath g_user_data_dir_for_test_;

  bool registered_ = false;

  base::ObserverList<Observer> observers_;
  base::FilePath user_data_dir_;
  base::FilePath universal_qa_model_path_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEO_LOCAL_MODELS_UPDATER_H_
