/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_LOCAL_MODELS_UPDATER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_LOCAL_MODELS_UPDATER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "components/component_updater/component_installer.h"
#include "components/update_client/update_client.h"

namespace base {
class Version;
template <typename T>
class NoDestructor;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace local_ai {

inline constexpr char kEmbeddingGemmaModelDir[] = "embeddinggemma-300m";
inline constexpr char kEmbeddingGemmaModelFile[] = "model.gguf";
inline constexpr char kEmbeddingGemmaConfigFile[] = "config.json";
inline constexpr char kEmbeddingGemmaTokenizerFile[] = "tokenizer.json";

// Exposed for testing - follows upstream Chromium pattern.
class LocalModelsComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  LocalModelsComponentInstallerPolicy();
  ~LocalModelsComponentInstallerPolicy() override;

  LocalModelsComponentInstallerPolicy(
      const LocalModelsComponentInstallerPolicy&) = delete;
  LocalModelsComponentInstallerPolicy& operator=(
      const LocalModelsComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy:
  bool VerifyInstallation(const base::Value::Dict& manifest,
                          const base::FilePath& install_dir) const override;
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value::Dict& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::Value::Dict manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;
};

class LocalModelsUpdaterState {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Called when the component installation directory is set/updated
    virtual void OnComponentReady(const base::FilePath& install_dir) = 0;
  };

  static LocalModelsUpdaterState* GetInstance();

  LocalModelsUpdaterState(const LocalModelsUpdaterState&) = delete;
  LocalModelsUpdaterState& operator=(const LocalModelsUpdaterState&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetInstallDir(const base::FilePath& install_dir);
  const base::FilePath& GetInstallDir() const;

  const base::FilePath& GetEmbeddingGemmaModelDir() const;
  const base::FilePath& GetEmbeddingGemmaModel() const;
  const base::FilePath& GetEmbeddingGemmaConfig() const;
  const base::FilePath& GetEmbeddingGemmaTokenizer() const;

 private:
  friend base::NoDestructor<LocalModelsUpdaterState>;
  LocalModelsUpdaterState();
  ~LocalModelsUpdaterState();

  base::FilePath install_dir_;
  base::FilePath embeddinggemma_model_dir_;
  base::FilePath embeddinggemma_model_path_;
  base::FilePath embeddinggemma_config_path_;
  base::FilePath embeddinggemma_tokenizer_path_;

  base::ObserverList<Observer> observers_;
};

void ManageLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_LOCAL_MODELS_UPDATER_H_
