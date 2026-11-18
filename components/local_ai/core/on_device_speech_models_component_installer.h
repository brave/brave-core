/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"

class PrefService;

namespace base {
class Version;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace local_ai {

// Component ids of Brave's on-device speech recognition models, each derived
// from the public key its component is signed with.
inline constexpr char kOnDeviceSpeechEnglishModelComponentId[] =
    "nhkekccefdppopbldokibkoegppanbba";
inline constexpr char kOnDeviceSpeechMultilingualModelComponentId[] =
    "gcanmpgghlnodnjdmialcjddcnloedij";

// Component Updater policy for one of Brave's on-device speech recognition
// models. Exposed for testing - follows upstream Chromium pattern.
class OnDeviceSpeechModelsComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  // `local_state` gates `ComponentReady` on `ShouldInstallOnDeviceSpeechModel`,
  // which can turn false while an update is already in flight.
  OnDeviceSpeechModelsComponentInstallerPolicy(OnDeviceSpeechModel model,
                                               PrefService* local_state);
  ~OnDeviceSpeechModelsComponentInstallerPolicy() override;

  OnDeviceSpeechModelsComponentInstallerPolicy(
      const OnDeviceSpeechModelsComponentInstallerPolicy&) = delete;
  OnDeviceSpeechModelsComponentInstallerPolicy& operator=(
      const OnDeviceSpeechModelsComponentInstallerPolicy&) = delete;

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
  const OnDeviceSpeechModel model_;
  raw_ptr<PrefService> local_state_ = nullptr;
};

// Called once, while components are registered at startup. Sets up a
// registrar per model, which from then on follows the `kBraveLocalAIEnabled`
// master switch, `kBraveOnDeviceSpeechRecognition` and the model's own enabled
// pref (`GetOnDeviceSpeechModelEnabledPref`) for the rest of the session. The
// multilingual registrar also follows `kBraveOnDeviceSpeechMultilingual`. Each
// registers its model only while that model is enabled, and otherwise takes
// it back off disk. Turning the switch or `kBraveOnDeviceSpeechRecognition` off
// also clears both enabled prefs. `kBraveOnDeviceSpeechMultilingual` off clears
// only the multilingual pref.
//
// Brave Origin manages the switch and verifies the purchase asynchronously, so
// its value can land after components are registered, which is why the switch
// is followed rather than read once.
//
// `local_state` is required. `cus` may be null, which leaves the registrar
// with nothing to register against and every request refused. Calling this
// again without `ShutdownOnDeviceSpeechModelsComponentRegistration` first
// CHECK-fails.
void ManageOnDeviceSpeechModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_state);

// Drops the references taken by
// `ManageOnDeviceSpeechModelsComponentRegistration`. Must run before `cus` and
// `local_state` are destroyed.
void ShutdownOnDeviceSpeechModelsComponentRegistration();

// Registers `model`'s component, publishing a copy already on disk and asking
// the updater to download one that is not. Safe to call repeatedly.
// `callback` runs once, asynchronously, with whether `model` ended up
// installed, even when nothing is registered, which is the case while the
// feature or the master switch is off, while `kBraveOnDeviceSpeechMultilingual`
// is off for the multilingual model, while `model` is not enabled
// (`GetOnDeviceSpeechModelEnabledPref`), or before
// `ManageOnDeviceSpeechModelsComponentRegistration` has run.
void MaybeRegisterOnDeviceSpeechModelsComponent(
    OnDeviceSpeechModel model,
    base::OnceCallback<void(bool)> callback);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_
