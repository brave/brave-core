/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"

#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "crypto/sha2.h"

namespace local_ai {

namespace {

constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveOnDeviceSpeechModels");
constexpr char kComponentName[] = "Brave On-Device Speech Models";

// SHA256 of the provisioned component's public key.
constexpr uint8_t kPublicKeySHA256[32] = {
    0xd7, 0xa4, 0xa2, 0x24, 0x53, 0xff, 0xef, 0x1b, 0x3e, 0xa8, 0x1a,
    0xe4, 0x6f, 0xf0, 0xd1, 0x10, 0xac, 0xaa, 0x39, 0x3b, 0x03, 0xcd,
    0xf1, 0x10, 0x04, 0x5e, 0xf9, 0x33, 0xf5, 0xe9, 0x6c, 0x4d};
static_assert(std::size(kPublicKeySHA256) == crypto::kSHA256Length,
              "Wrong hash length");

base::FilePath GetComponentDir() {
  base::FilePath components_dir =
      base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);

  return components_dir.Append(kComponentInstallDir);
}

void DeleteComponentDirectory() {
  // Posted because removing the model is hundreds of megabytes of blocking
  // file I/O.
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(base::IgnoreResult(&base::DeletePathRecursively),
                     GetComponentDir()));
}

// Whether the component may be installed. The feature is fixed for the session
// but `kBraveLocalAIEnabled` is managed by Brave Origin and can flip at any
// time.
bool IsComponentAllowed(const PrefService* local_state) {
  return local_state &&
         base::FeatureList::IsEnabled(kBraveOnDeviceSpeechRecognition) &&
         local_state->GetBoolean(prefs::kBraveLocalAIEnabled);
}

// Watches the master switch so the component stays in sync with it for the
// whole session. `MaybeRegisterOnDeviceSpeechModelsComponent` still decides
// what a given state means, including which of its guards wins, so this only
// adds the pref subscription and the deregistration that a one-shot caller has
// no reason to do.
class OnDeviceSpeechModelsComponentRegistrar {
 public:
  static OnDeviceSpeechModelsComponentRegistrar* GetInstance() {
    static base::NoDestructor<OnDeviceSpeechModelsComponentRegistrar> instance;
    return instance.get();
  }

  OnDeviceSpeechModelsComponentRegistrar(
      const OnDeviceSpeechModelsComponentRegistrar&) = delete;
  OnDeviceSpeechModelsComponentRegistrar& operator=(
      const OnDeviceSpeechModelsComponentRegistrar&) = delete;

  // Once per process, or once per `Shutdown`.
  void Start(component_updater::ComponentUpdateService* cus,
             PrefService* local_state) {
    CHECK(pref_change_registrar_.IsEmpty() && !cus_);
    CHECK(local_state);
    cus_ = cus;
    pref_change_registrar_.Init(local_state);
    pref_change_registrar_.Add(
        prefs::kBraveLocalAIEnabled,
        base::BindRepeating(&OnDeviceSpeechModelsComponentRegistrar::Sync,
                            base::Unretained(this)));
    Sync();
  }

  void Shutdown() {
    pref_change_registrar_.Reset();
    cus_ = nullptr;
  }

 private:
  friend base::NoDestructor<OnDeviceSpeechModelsComponentRegistrar>;

  OnDeviceSpeechModelsComponentRegistrar() = default;
  ~OnDeviceSpeechModelsComponentRegistrar() = default;

  void Sync() {
    PrefService* local_state = pref_change_registrar_.prefs();
    if (!IsComponentAllowed(local_state)) {
      Unregister();
      return;
    }
    MaybeRegisterOnDeviceSpeechModelsComponent(cus_, local_state);
  }

  // Removing the model is not an update, so none of this asks the update
  // service for permission or needs one to exist. Registration is the only
  // thing that does.
  void Unregister() {
    if (cus_) {
      // Unregistering is what stops the updater bringing the model back.
      cus_->UnregisterComponent(kOnDeviceSpeechModelsComponentId);
    }
    // Published before the delete is posted, so nothing acts on a model whose
    // files are on their way out. A delete that lags or fails is caught by
    // `VerifyInstallation` at the next startup.
    OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(base::FilePath());
    DeleteComponentDirectory();
  }

  raw_ptr<component_updater::ComponentUpdateService> cus_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace

OnDeviceSpeechModelsComponentInstallerPolicy::
    OnDeviceSpeechModelsComponentInstallerPolicy(PrefService* local_state)
    : local_state_(local_state) {}
OnDeviceSpeechModelsComponentInstallerPolicy::
    ~OnDeviceSpeechModelsComponentInstallerPolicy() = default;

bool OnDeviceSpeechModelsComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return base::DirectoryExists(install_dir.AppendASCII(kModelDirName));
}

bool OnDeviceSpeechModelsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool OnDeviceSpeechModelsComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
OnDeviceSpeechModelsComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void OnDeviceSpeechModelsComponentInstallerPolicy::OnCustomUninstall() {}

void OnDeviceSpeechModelsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  if (install_dir.empty()) {
    return;
  }
  // Unregistration is deferred behind an in-flight update, so this still fires
  // for a download that started before the switch turned off.
  if (!IsComponentAllowed(local_state_)) {
    return;
  }
  OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(install_dir);
}

base::FilePath
OnDeviceSpeechModelsComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(kComponentInstallDir);
}

void OnDeviceSpeechModelsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(std::begin(kPublicKeySHA256), std::end(kPublicKeySHA256));
}

std::string OnDeviceSpeechModelsComponentInstallerPolicy::GetName() const {
  return kComponentName;
}

update_client::InstallerAttributes
OnDeviceSpeechModelsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool OnDeviceSpeechModelsComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void ManageOnDeviceSpeechModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_state) {
  OnDeviceSpeechModelsComponentRegistrar::GetInstance()->Start(cus,
                                                               local_state);
}

void ShutdownOnDeviceSpeechModelsComponentRegistration() {
  OnDeviceSpeechModelsComponentRegistrar::GetInstance()->Shutdown();
}

void MaybeRegisterOnDeviceSpeechModelsComponent(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_state,
    component_updater::Callback callback) {
  // Keep delivery consistently asynchronous across all branches below.
  callback = base::BindPostTaskToCurrentDefault(std::move(callback));

  CHECK(local_state);

  if (!IsComponentAllowed(local_state) || !cus) {
    std::move(callback).Run(update_client::Error::INVALID_ARGUMENT);
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<OnDeviceSpeechModelsComponentInstallerPolicy>(
          local_state));
  installer->Register(
      cus,
      base::BindOnce(
          [](component_updater::ComponentUpdateService* cus,
             PrefService* local_state, component_updater::Callback callback) {
            // The switch can have turned off while the registration was in
            // flight. Take the component back out here, because it only
            // reaches the update service now, so the unregister that turning
            // the switch off ran found nothing to remove.
            if (!IsComponentAllowed(local_state)) {
              cus->UnregisterComponent(kOnDeviceSpeechModelsComponentId);
              std::move(callback).Run(update_client::Error::INVALID_ARGUMENT);
              return;
            }
            // Registering has already published whatever was on disk, so a
            // copy already downloaded stays usable. Only the download is
            // refused, which is the part --disable-component-update forbids,
            // and asking for it anyway trips a DCHECK in BraveOnDemandUpdater.
            if (brave_component_updater::BraveOnDemandUpdater::GetInstance()
                    ->is_component_update_disabled()) {
              std::move(callback).Run(update_client::Error::INVALID_ARGUMENT);
              return;
            }
            brave_component_updater::BraveOnDemandUpdater::GetInstance()
                ->EnsureInstalled(kOnDeviceSpeechModelsComponentId,
                                  std::move(callback));
          },
          cus, local_state, std::move(callback)));
}

}  // namespace local_ai
