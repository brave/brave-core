/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/local_models_updater.h"

#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/update_client.h"
#include "crypto/sha2.h"

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
static_assert(std::size(kPublicKeySHA256) == crypto::kSHA256Length,
              "Wrong hash length");

base::FilePath GetComponentDir() {
  base::FilePath components_dir =
      base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);

  return components_dir.Append(kComponentInstallDir);
}

void DeleteComponentDirectory() {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::GetDeletePathRecursivelyCallback(GetComponentDir()));
}

// Keeps the component registration in sync with the `kBraveLocalAIEnabled`
// master switch for the whole session, so a switch that only turns off after
// components are registered still tears the component back down.
class LocalModelsComponentRegistrar {
 public:
  static LocalModelsComponentRegistrar* GetInstance() {
    static base::NoDestructor<LocalModelsComponentRegistrar> instance;
    return instance.get();
  }

  LocalModelsComponentRegistrar(const LocalModelsComponentRegistrar&) = delete;
  LocalModelsComponentRegistrar& operator=(
      const LocalModelsComponentRegistrar&) = delete;

  // At most once per process, or once per Shutdown(): this is a singleton and
  // PrefChangeRegistrar DCHECKs when the same pref is registered twice.
  void Start(component_updater::ComponentUpdateService* cus,
             PrefService* local_state) {
    CHECK(pref_change_registrar_.IsEmpty());
    cus_ = cus;
    if (local_state) {
      pref_change_registrar_.Init(local_state);
      pref_change_registrar_.Add(
          prefs::kBraveLocalAIEnabled,
          base::BindRepeating(&LocalModelsComponentRegistrar::Sync,
                              base::Unretained(this)));
    }
    Sync();
  }

  void Shutdown() {
    pref_change_registrar_.Reset();
    cus_ = nullptr;
  }

 private:
  friend base::NoDestructor<LocalModelsComponentRegistrar>;

  LocalModelsComponentRegistrar() = default;
  ~LocalModelsComponentRegistrar() = default;

  bool IsEnabled() const {
    const PrefService* local_state = pref_change_registrar_.prefs();
    return cus_ && local_state &&
           local_state->GetBoolean(prefs::kBraveLocalAIEnabled) &&
           base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings);
  }

  void Sync() {
    if (!IsEnabled()) {
      Unregister();
      return;
    }

    auto installer =
        base::MakeRefCounted<component_updater::ComponentInstaller>(
            std::make_unique<LocalModelsComponentInstallerPolicy>());
    installer->Register(
        cus_, base::BindOnce(&LocalModelsComponentRegistrar::OnRegistered,
                             base::Unretained(this)));
  }

  // ComponentInstaller::Register() reads the installed manifest on a blocking
  // task runner before it registers, so the master switch can turn off in
  // between - at which point Sync() has nothing to unregister yet.
  void OnRegistered() {
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
    if (cus_) {
      cus_->UnregisterComponent(kComponentId);
    }
    LocalModelsUpdaterState::GetInstance()->SetInstallDir(base::FilePath());
    DeleteComponentDirectory();
  }

  raw_ptr<component_updater::ComponentUpdateService> cus_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace

LocalModelsComponentInstallerPolicy::LocalModelsComponentInstallerPolicy() =
    default;
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
    PrefService* local_state) {
  LocalModelsComponentRegistrar::GetInstance()->Start(cus, local_state);
}

void ShutdownLocalModelsComponentRegistration() {
  LocalModelsComponentRegistrar::GetInstance()->Shutdown();
}

}  // namespace local_ai
