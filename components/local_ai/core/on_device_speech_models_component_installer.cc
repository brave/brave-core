/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/scoped_observation.h"
#include "base/task/bind_post_task.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/local_ai/core/utils.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "crypto/hash.h"

namespace local_ai {

namespace {

struct ComponentConfig {
  std::string_view id;
  // SHA256 of the component's public key.
  std::array<uint8_t, crypto::hash::kSha256Size> public_key_sha256;
  base::FilePath::StringViewType install_dir;
  std::string_view name;
};

const ComponentConfig& GetComponentConfig(OnDeviceSpeechModel model) {
  static constexpr ComponentConfig kEnglishConfig = {
      .id = kOnDeviceSpeechEnglishModelComponentId,
      .public_key_sha256 = {0xd7, 0xa4, 0xa2, 0x24, 0x53, 0xff, 0xef, 0x1b,
                            0x3e, 0xa8, 0x1a, 0xe4, 0x6f, 0xf0, 0xd1, 0x10,
                            0xac, 0xaa, 0x39, 0x3b, 0x03, 0xcd, 0xf1, 0x10,
                            0x04, 0x5e, 0xf9, 0x33, 0xf5, 0xe9, 0x6c, 0x4d},
      .install_dir = FILE_PATH_LITERAL("BraveOnDeviceSpeechModels"),
      .name = "Brave On-Device Speech Models",
  };
  static constexpr ComponentConfig kMultilingualConfig = {
      .id = kOnDeviceSpeechMultilingualModelComponentId,
      .public_key_sha256 = {0x62, 0x0d, 0xcf, 0x66, 0x7b, 0xde, 0x3d, 0x93,
                            0xc8, 0x0b, 0x29, 0x33, 0x2d, 0xbe, 0x43, 0x89,
                            0x49, 0xe0, 0xcc, 0xf8, 0x75, 0x76, 0x26, 0x41,
                            0xcd, 0xaf, 0xbf, 0x64, 0xfb, 0x20, 0xfe, 0x70},
      .install_dir = FILE_PATH_LITERAL("BraveOnDeviceSpeechMultilingualModels"),
      .name = "Brave On-Device Speech Multilingual Models",
  };
  switch (model) {
    case OnDeviceSpeechModel::kEnglish:
      return kEnglishConfig;
    case OnDeviceSpeechModel::kMultilingual:
      return kMultilingualConfig;
  }
  NOTREACHED();
}

// Owns one model's component registration for the whole session and follows
// the master switch and the model's own enabled pref, so the model is on disk
// only while `ShouldInstallOnDeviceSpeechModel` is true for it. It is also the
// one place that decides when an install request for the model is over, and
// answers it with whether the model is installed.
//
// Registering and removing the model both go through the one
// `ComponentInstaller` this holds, which is what orders a removal against an
// install rather than racing it on an unrelated sequence.
class ModelComponentRegistrar : public component_updater::ServiceObserver {
 public:
  explicit ModelComponentRegistrar(OnDeviceSpeechModel model) : model_(model) {}
  ~ModelComponentRegistrar() override = default;

  ModelComponentRegistrar(const ModelComponentRegistrar&) = delete;
  ModelComponentRegistrar& operator=(const ModelComponentRegistrar&) = delete;

  // Once per process, or once per `Shutdown`.
  void Start(component_updater::ComponentUpdateService* cus,
             PrefService* local_state) {
    CHECK(pref_change_registrar_.IsEmpty() && !cus_);
    CHECK(local_state);
    cus_ = cus;
    pref_change_registrar_.Init(local_state);
    pref_change_registrar_.Add(
        prefs::kBraveLocalAIEnabled,
        base::BindRepeating(&ModelComponentRegistrar::Sync,
                            base::Unretained(this)));
    pref_change_registrar_.Add(
        GetOnDeviceSpeechModelEnabledPref(model_),
        base::BindRepeating(&ModelComponentRegistrar::Sync,
                            base::Unretained(this)));
    Sync();
  }

  void Shutdown() {
    pref_change_registrar_.Reset();
    cus_ = nullptr;
    installer_.reset();
    registration_pending_ = false;
    // No model can arrive once the update service has been let go of, so a
    // request still waiting is over.
    SettlePendingRequests();
  }

  // Registers the component, which also requests the download. Joins a
  // registration already in flight instead of starting a second one.
  void Register(base::OnceCallback<void(bool)> callback) {
    // Answered from a task on every path, so an answer never lands inside the
    // caller's own call.
    callback = base::BindPostTaskToCurrentDefault(std::move(callback));

    if (!cus_ || !ShouldInstallOnDeviceSpeechModel(
                     pref_change_registrar_.prefs(), model_)) {
      std::move(callback).Run(false);
      return;
    }

    pending_callbacks_.push_back(std::move(callback));
    if (registration_pending_) {
      return;
    }
    registration_pending_ = true;
    EnsureInstaller();
    installer_->Register(cus_,
                         base::BindOnce(&ModelComponentRegistrar::OnRegistered,
                                        base::Unretained(this)));
  }

 private:
  const ComponentConfig& config() const { return GetComponentConfig(model_); }

  void Sync() {
    PrefService* local_state = pref_change_registrar_.prefs();
    if (!ShouldInstallOnDeviceSpeechModel(local_state, model_)) {
      const std::string_view enabled_pref =
          GetOnDeviceSpeechModelEnabledPref(model_);
      // The switch or a feature took the model away. Enabling the model does
      // not outlive them, so turning them back on asks again rather than
      // downloading. Clearing it runs `Sync` again, and that run removes the
      // model.
      if (local_state->GetBoolean(enabled_pref)) {
        local_state->ClearPref(enabled_pref);
        return;
      }
      Unregister();
      return;
    }
    Register(base::DoNothing());
  }

  void OnRegistered() {
    registration_pending_ = false;
    // `Shutdown` ran while this was in flight. Falling through would read the
    // prefs it dropped as the master switch being off and remove the model.
    if (!cus_) {
      SettlePendingRequests();
      return;
    }
    // The switch turned off, or the model was disabled, while this was in
    // flight, so the unregister it ran found nothing to remove. Nothing else
    // will try again, so finish the removal here.
    if (!ShouldInstallOnDeviceSpeechModel(pref_change_registrar_.prefs(),
                                          model_)) {
      Unregister();
      return;
    }
    // Registering already published whatever was on disk. Only the download is
    // refused, which is what --disable-component-update forbids and what trips
    // a DCHECK in BraveOnDemandUpdater.
    if (brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->is_component_update_disabled()) {
      SettlePendingRequests();
      return;
    }
    // Watched before the download is asked for, so an event it causes cannot
    // be missed. A request made while one is already running starts nothing
    // and is answered `UPDATE_IN_PROGRESS`, so how that download ended is
    // only reported on `ServiceObserver::OnEvent`.
    if (!cus_observation_.IsObserving()) {
      cus_observation_.Observe(cus_);
    }
    brave_component_updater::BraveOnDemandUpdater::GetInstance()
        ->EnsureInstalled(
            std::string(config().id),
            base::BindOnce(&ModelComponentRegistrar::OnEnsureInstalled,
                           base::Unretained(this)));
  }

  void OnEnsureInstalled(update_client::Error error) {
    // `UPDATE_IN_PROGRESS` leaves the request waiting on
    // `ServiceObserver::OnEvent`. Every other answer ends it, including `NONE`
    // for an update that never started because a copy was already installed.
    if (error == update_client::Error::UPDATE_IN_PROGRESS) {
      return;
    }
    SettlePendingRequests();
  }

  // component_updater::ServiceObserver:
  // Only ever watched for a request waiting on a download, and every drain
  // stops watching, so there is always one waiting here.
  void OnEvent(const update_client::CrxUpdateItem& item) override {
    if (item.id != config().id) {
      return;
    }
    // The first state an update cannot leave settles what is waiting. Which
    // state it is says nothing on its own, because `kUpdated` and `kUpToDate`
    // can both leave no model behind.
    if (item.state == update_client::ComponentState::kUpdated ||
        item.state == update_client::ComponentState::kUpToDate ||
        item.state == update_client::ComponentState::kUpdateError) {
      SettlePendingRequests();
    }
  }

  void Unregister() {
    // Only remove the files ourselves when the service did not (it uninstalls
    // what it had registered) and no registration in flight may still install
    // them.
    const bool was_registered =
        cus_ && cus_->UnregisterComponent(std::string(config().id));
    // Published before the files go, so nothing acts on a model whose files
    // are on their way out.
    OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(model_,
                                                            base::FilePath());
    if (!was_registered && !registration_pending_) {
      EnsureInstaller();
      installer_->Uninstall();
    }
    // A registration still in flight reads the prefs again when it lands,
    // which is what lets a request made again before then finish the install
    // it started. With none in flight, nothing else will answer what is
    // waiting.
    if (!registration_pending_) {
      SettlePendingRequests();
    }
  }

  // Answers every request waiting on this registration, and stops watching the
  // update service, which is only ever watched for them.
  void SettlePendingRequests() {
    cus_observation_.Reset();
    if (pending_callbacks_.empty()) {
      return;
    }
    const bool success =
        OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled(model_);
    std::vector<base::OnceCallback<void(bool)>> callbacks;
    callbacks.swap(pending_callbacks_);
    for (auto& callback : callbacks) {
      std::move(callback).Run(success);
    }
  }

  void EnsureInstaller() {
    if (!installer_) {
      installer_ = base::MakeRefCounted<component_updater::ComponentInstaller>(
          std::make_unique<OnDeviceSpeechModelsComponentInstallerPolicy>(
              model_, pref_change_registrar_.prefs()));
    }
  }

  const OnDeviceSpeechModel model_;
  // Held from `Start` until `Shutdown`, which is also what tells an in-flight
  // registration that it landed too late to be finished.
  raw_ptr<component_updater::ComponentUpdateService> cus_ = nullptr;
  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ServiceObserver>
      cus_observation_{this};
  PrefChangeRegistrar pref_change_registrar_;
  // True from `Register` until `OnRegistered`. The component is absent from
  // the update service for that whole window, so a second `Register` would
  // register it twice and an `Unregister` would find nothing to unregister.
  bool registration_pending_ = false;
  // Answered together, because everyone who asked while one registration was
  // in flight is waiting on that same registration.
  std::vector<base::OnceCallback<void(bool)>> pending_callbacks_;
  // Reused: registration and uninstall share the installer's task runner, so a
  // per-registration instance would let an uninstall delete what the next
  // registration installed.
  scoped_refptr<component_updater::ComponentInstaller> installer_;
};

// Singleton holding the registrar of each of Brave's on-device speech
// recognition models.
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
    english_.Start(cus, local_state);
    multilingual_.Start(cus, local_state);
  }

  void Shutdown() {
    english_.Shutdown();
    multilingual_.Shutdown();
  }

  void Register(OnDeviceSpeechModel model,
                base::OnceCallback<void(bool)> callback) {
    GetRegistrar(model).Register(std::move(callback));
  }

 private:
  friend base::NoDestructor<OnDeviceSpeechModelsComponentRegistrar>;

  OnDeviceSpeechModelsComponentRegistrar() = default;
  ~OnDeviceSpeechModelsComponentRegistrar() = default;

  ModelComponentRegistrar& GetRegistrar(OnDeviceSpeechModel model) {
    switch (model) {
      case OnDeviceSpeechModel::kEnglish:
        return english_;
      case OnDeviceSpeechModel::kMultilingual:
        return multilingual_;
    }
    NOTREACHED();
  }

  ModelComponentRegistrar english_{OnDeviceSpeechModel::kEnglish};
  ModelComponentRegistrar multilingual_{OnDeviceSpeechModel::kMultilingual};
};

}  // namespace

OnDeviceSpeechModelsComponentInstallerPolicy::
    OnDeviceSpeechModelsComponentInstallerPolicy(OnDeviceSpeechModel model,
                                                 PrefService* local_state)
    : model_(model), local_state_(local_state) {}
OnDeviceSpeechModelsComponentInstallerPolicy::
    ~OnDeviceSpeechModelsComponentInstallerPolicy() = default;

bool OnDeviceSpeechModelsComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return base::DirectoryExists(
      install_dir.AppendASCII(GetModelDirName(model_)));
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
  // for a download that started before the switch turned off or the model was
  // disabled.
  if (!ShouldInstallOnDeviceSpeechModel(local_state_, model_)) {
    return;
  }
  OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(model_, install_dir);
}

base::FilePath
OnDeviceSpeechModelsComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(GetComponentConfig(model_).install_dir);
}

void OnDeviceSpeechModelsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  const auto& public_key_sha256 = GetComponentConfig(model_).public_key_sha256;
  hash->assign(public_key_sha256.begin(), public_key_sha256.end());
}

std::string OnDeviceSpeechModelsComponentInstallerPolicy::GetName() const {
  return std::string(GetComponentConfig(model_).name);
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
    OnDeviceSpeechModel model,
    base::OnceCallback<void(bool)> callback) {
  OnDeviceSpeechModelsComponentRegistrar::GetInstance()->Register(
      model, std::move(callback));
}

}  // namespace local_ai
