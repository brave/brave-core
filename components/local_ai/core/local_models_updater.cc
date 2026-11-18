/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/local_models_updater.h"

#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/update_client/update_client.h"
#include "crypto/hash.h"

namespace local_ai {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveLocalAIModels");
constexpr char kComponentName[] = "Brave Local AI Models Updater";
constexpr char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";
constexpr uint8_t kPublicKeySHA256[32] = {
    0x49, 0x74, 0x99, 0xc0, 0xe0, 0xe7, 0xf6, 0x7d, 0x1b, 0x23, 0x29,
    0x8b, 0xd3, 0xa0, 0xd6, 0x54, 0xb6, 0xc3, 0x23, 0x87, 0x75, 0xec,
    0x54, 0x78, 0x1d, 0x83, 0xf4, 0xc3, 0xeb, 0x6d, 0x70, 0xb6};
static_assert(std::size(kPublicKeySHA256) == crypto::hash::kSha256Size,
              "Wrong hash length");

// Whether the model is to be on disk: the master switch is on and at least one
// profile turned Semantic history search on. Without a `local_state` there is
// no record of any profile opting in, so the model is not installed.
bool ShouldInstallLocalModels(const PrefService* local_state) {
  return local_state && local_state->GetBoolean(prefs::kBraveLocalAIEnabled) &&
         !local_state->GetList(prefs::kHistoryEmbeddingsEnabledProfiles)
              .empty();
}

// Keeps the component registration in sync with the `kBraveLocalAIEnabled`
// master switch, which Brave Origin can flip long after startup, and with
// `kHistoryEmbeddingsEnabledProfiles`, so the model is on disk only while a
// profile uses it.
class LocalModelsComponentRegistrar {
 public:
  static LocalModelsComponentRegistrar* GetInstance() {
    static base::NoDestructor<LocalModelsComponentRegistrar> instance;
    return instance.get();
  }

  LocalModelsComponentRegistrar(const LocalModelsComponentRegistrar&) = delete;
  LocalModelsComponentRegistrar& operator=(
      const LocalModelsComponentRegistrar&) = delete;

  // Once per process, or once per Shutdown().
  void Start(component_updater::ComponentUpdateService* cus,
             PrefService* local_state) {
    CHECK(pref_change_registrar_.IsEmpty() && !cus_);
    cus_ = cus;
    if (local_state) {
      pref_change_registrar_.Init(local_state);
      pref_change_registrar_.Add(
          prefs::kBraveLocalAIEnabled,
          base::BindRepeating(&LocalModelsComponentRegistrar::Sync,
                              base::Unretained(this)));
      pref_change_registrar_.Add(
          prefs::kHistoryEmbeddingsEnabledProfiles,
          base::BindRepeating(&LocalModelsComponentRegistrar::Sync,
                              base::Unretained(this)));
    }
    Sync();
  }

  void Shutdown() {
    pref_change_registrar_.Reset();
    cus_ = nullptr;
    installer_.reset();
    registration_pending_ = false;
  }

 private:
  friend base::NoDestructor<LocalModelsComponentRegistrar>;

  LocalModelsComponentRegistrar() = default;
  ~LocalModelsComponentRegistrar() = default;

  bool IsEnabled() const {
    return cus_ && ShouldInstallLocalModels(pref_change_registrar_.prefs()) &&
           base::FeatureList::IsEnabled(
               brave_history_embeddings::kHistoryEmbeddings);
  }

  void Sync() {
    if (!IsEnabled()) {
      Unregister();
      return;
    }
    Register();
  }

  void Register() {
    if (registration_pending_) {
      return;
    }
    registration_pending_ = true;
    EnsureInstaller();
    installer_->Register(
        cus_, base::BindOnce(&LocalModelsComponentRegistrar::OnRegistered,
                             base::Unretained(this)));
  }

  // The switch can have turned off while the registration was in flight.
  void OnRegistered() {
    registration_pending_ = false;
    if (!cus_) {
      return;
    }
    if (!IsEnabled()) {
      Unregister();
      return;
    }
    brave_component_updater::BraveOnDemandUpdater::GetInstance()
        ->EnsureInstalled(kComponentId);
  }

  void Unregister() {
    // Only remove it ourselves when the service did not (it uninstalls what it
    // had registered) and no registration in flight may still publish it.
    const bool was_registered = cus_ && cus_->UnregisterComponent(kComponentId);
    LocalModelsUpdaterState::GetInstance()->SetInstallDir(base::FilePath());
    if (!was_registered && !registration_pending_) {
      EnsureInstaller();
      installer_->Uninstall();
    }
  }

  void EnsureInstaller() {
    if (!installer_) {
      installer_ = base::MakeRefCounted<component_updater::ComponentInstaller>(
          std::make_unique<LocalModelsComponentInstallerPolicy>(
              pref_change_registrar_.prefs()));
    }
  }

  raw_ptr<component_updater::ComponentUpdateService> cus_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  // True from Register() until OnRegistered(). The component is absent from
  // the service for that whole window, so a second Register() would register
  // it twice and an Unregister() would find nothing to unregister.
  bool registration_pending_ = false;
  // Reused: registration and uninstall share the installer's task runner, so a
  // per-registration instance would let an uninstall delete what the next
  // registration installed.
  scoped_refptr<component_updater::ComponentInstaller> installer_;
};

}  // namespace

LocalModelsComponentInstallerPolicy::LocalModelsComponentInstallerPolicy(
    PrefService* local_state)
    : local_state_(local_state) {}
LocalModelsComponentInstallerPolicy::~LocalModelsComponentInstallerPolicy() =
    default;

bool LocalModelsComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

bool LocalModelsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool LocalModelsComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
LocalModelsComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void LocalModelsComponentInstallerPolicy::OnCustomUninstall() {}

void LocalModelsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  if (install_dir.empty()) {
    return;
  }
  // Unregistration is deferred behind an in-flight update, so this still fires
  // for a download that started before the switch turned off or the last
  // profile turned Semantic history search off.
  if (!ShouldInstallLocalModels(local_state_)) {
    return;
  }
  LocalModelsUpdaterState::GetInstance()->SetInstallDir(install_dir);
}

base::FilePath LocalModelsComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath(kComponentInstallDir);
}

void LocalModelsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(std::begin(kPublicKeySHA256), std::end(kPublicKeySHA256));
}

std::string LocalModelsComponentInstallerPolicy::GetName() const {
  return kComponentName;
}

update_client::InstallerAttributes
LocalModelsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool LocalModelsComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

LocalModelsUpdaterState* LocalModelsUpdaterState::GetInstance() {
  static base::NoDestructor<LocalModelsUpdaterState> instance;
  return instance.get();
}

void LocalModelsUpdaterState::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);

  // If component is already ready, notify immediately
  if (!install_dir_.empty()) {
    observer->OnLocalModelsReady(install_dir_);
  }
}

void LocalModelsUpdaterState::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void LocalModelsUpdaterState::SetInstallDir(const base::FilePath& install_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (install_dir_ == install_dir) {
    return;
  }
  install_dir_ = install_dir;
  if (install_dir.empty()) {
    embeddinggemma_litert_dir_ = base::FilePath();
    observers_.Notify(&Observer::OnLocalModelsUnavailable);
    return;
  }
  embeddinggemma_litert_dir_ = install_dir_.AppendASCII(kEmbeddingGemmaModelDir)
                                   .AppendASCII(kEmbeddingGemmaLitertDir);

  observers_.Notify(&Observer::OnLocalModelsReady, install_dir_);
}

const base::FilePath& LocalModelsUpdaterState::GetInstallDir() const {
  return install_dir_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaLitertDir()
    const {
  return embeddinggemma_litert_dir_;
}

LocalModelsUpdaterState::LocalModelsUpdaterState() = default;
LocalModelsUpdaterState::~LocalModelsUpdaterState() = default;

void ManageLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_state,
    const std::optional<base::flat_set<std::string>>& profile_names) {
  if (local_state && profile_names) {
    ScopedListPrefUpdate update(local_state,
                                prefs::kHistoryEmbeddingsEnabledProfiles);
    update->EraseIf([&profile_names](const base::Value& name) {
      return !name.is_string() || !profile_names->contains(name.GetString());
    });
  }
  LocalModelsComponentRegistrar::GetInstance()->Start(cus, local_state);
}

void ShutdownLocalModelsComponentRegistration() {
  LocalModelsComponentRegistrar::GetInstance()->Shutdown();
}

void SetHistoryEmbeddingsEnabledForProfile(PrefService& local_state,
                                           std::string_view profile_name,
                                           bool enabled) {
  // Skipping a write that changes nothing keeps the registrar from syncing on
  // every profile load.
  if (local_state.GetList(prefs::kHistoryEmbeddingsEnabledProfiles)
          .contains(profile_name) == enabled) {
    return;
  }
  ScopedListPrefUpdate update(&local_state,
                              prefs::kHistoryEmbeddingsEnabledProfiles);
  if (enabled) {
    update->Append(profile_name);
  } else {
    update->EraseValue(base::Value(profile_name));
  }
}

}  // namespace local_ai
