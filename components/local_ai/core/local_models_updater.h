/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_MODELS_UPDATER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_MODELS_UPDATER_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "components/component_updater/component_installer.h"
#include "components/update_client/update_client.h"

namespace base {
class Version;
template <typename T>
class NoDestructor;
}  // namespace base

class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace local_ai {

inline constexpr char kEmbeddingGemmaModelDir[] = "embeddinggemma-300m";
// The LiteRT model files sit in a subdir of the model dir, and stay there: the
// component keeps shipping the files at the top level for the older Brave
// versions that still read them.
inline constexpr char kEmbeddingGemmaLitertDir[] = "litert";

// Exposed for testing - follows upstream Chromium pattern.
class LocalModelsComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  // `local_state` gates ComponentReady() on the `kBraveLocalAIEnabled` master
  // switch.
  explicit LocalModelsComponentInstallerPolicy(PrefService* local_state);
  ~LocalModelsComponentInstallerPolicy() override;

  LocalModelsComponentInstallerPolicy(
      const LocalModelsComponentInstallerPolicy&) = delete;
  LocalModelsComponentInstallerPolicy& operator=(
      const LocalModelsComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy:
  bool VerifyInstallation(const base::DictValue& manifest,
                          const base::FilePath& install_dir) const override;
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;

 private:
  raw_ptr<PrefService> local_state_ = nullptr;
};

class LocalModelsUpdaterState {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Called when the local models are ready (component installed)
    virtual void OnLocalModelsReady(const base::FilePath& install_dir) = 0;

    // Called when the local models are gone. Observers must drop anything
    // they derived from the install dir: those files no longer exist.
    virtual void OnLocalModelsUnavailable() = 0;
  };

  static LocalModelsUpdaterState* GetInstance();

  LocalModelsUpdaterState(const LocalModelsUpdaterState&) = delete;
  LocalModelsUpdaterState& operator=(const LocalModelsUpdaterState&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetInstallDir(const base::FilePath& install_dir);
  const base::FilePath& GetInstallDir() const;

  // Dir the LiteRT EmbeddingGemma model ships in, holding model.tflite,
  // model-info.pb and the SentencePiece model.
  const base::FilePath& GetEmbeddingGemmaLitertDir() const;

 private:
  friend base::NoDestructor<LocalModelsUpdaterState>;
  LocalModelsUpdaterState();
  ~LocalModelsUpdaterState();

  base::FilePath install_dir_;
  base::FilePath embeddinggemma_litert_dir_;

  base::ObserverList<Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
};

// Registers or unregisters the local models component to match the
// `kBraveLocalAIEnabled` master switch and
// `kHistoryEmbeddingsEnabledProfiles`, and keeps it in sync for the rest of the
// session. The switch is managed by Brave Origin, whose purchase check is
// asynchronous, so its value can land after components are registered.
//
// `profile_names` holds the base names of all profiles that exist. Before the
// first sync, entries of `kHistoryEmbeddingsEnabledProfiles` that are not in
// `profile_names` are removed. Profiles deleted in an earlier session then no
// longer count, and the model is removed if no remaining profile uses it. With
// no `profile_names`, which profiles exist is unknown, so nothing is removed.
void ManageLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_state,
    const std::optional<base::flat_set<std::string>>& profile_names);

// Drops the references taken by ManageLocalModelsComponentRegistration(). Must
// run before `cus` and `local_state` are destroyed.
void ShutdownLocalModelsComponentRegistration();

// Records in `prefs::kHistoryEmbeddingsEnabledProfiles` whether the profile
// with base name `profile_name` has Semantic history search on. The model is
// registered only while at least one profile does.
void SetHistoryEmbeddingsEnabledForProfile(PrefService& local_state,
                                           std::string_view profile_name,
                                           bool enabled);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_MODELS_UPDATER_H_
