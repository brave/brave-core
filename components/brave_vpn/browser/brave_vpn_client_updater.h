// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_CLIENT_UPDATER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_CLIENT_UPDATER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "build/build_config.h"
#include "components/component_updater/component_updater_service.h"

//class BraveVpnClientUpdaterTest;

using brave_component_updater::BraveComponent;

namespace brave_vpn {

#if BUILDFLAG(IS_WIN)
// TODO(bsclifton): does Windows CRX include all versions?
// or should there be x86 and arm64 specific versions?
static const char kBraveVpnClientComponentName[] = "Brave VPN Client Updater (Windows)";
static const char kBraveVpnClientComponentId[] = "ccebeokgmjohaelpmhicglfjdilmdhpi";
static const char kBraveVpnClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtymidxdTpjhE/"
    "efHs117EOqSpu8jUaOhRRcJBTvpGROlePWuwWwLImcLQmB5hlgbOH2v51c6FWSsdgH2SOE/"
    "UwYEypiSgctYzhMzxbqmsYwJMfVhhahuFtHUSokRMr8edgwo3DOpPV19m0jVfdbTgjn9bE7g9Z"
    "UrC/X45S+Wo23XogXjs2jz4Zgd3HHXWv8Y5cHShhh9byToGn/f/"
    "p8ikJiWrYVclwxfzW1ivjiJ+S+xyvxxbo+"
    "5cGeH3KVhH2IH5ubL9Q8wZjg7axvhDzbwINRd825Cp83q2PqgXBGc5q7JA53t5xR12YqofxhfL"
    "o+ztkmdRpHJ9XognuVUzA0uSwIDAQAB";
#else
// Not used on other platforms.
static const char kBraveVpnClientComponentName[] = "";
static const char kBraveVpnClientComponentId[] = "";
static const char kBraveVpnClientComponentBase64PublicKey[] = "";
#endif

class BraveVpnClientUpdater : public BraveComponent,
                              public BraveComponent::ComponentObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnExecutableReady(const base::FilePath& path) = 0;
    virtual void OnInstallationEvent(
        BraveComponent::ComponentObserver::Events event) = 0;

   protected:
    ~Observer() override = default;
  };

  explicit BraveVpnClientUpdater(BraveComponent::Delegate* delegate,
                                  const base::FilePath& user_data_dir);
  BraveVpnClientUpdater(const BraveVpnClientUpdater&) = delete;
  BraveVpnClientUpdater& operator=(const BraveVpnClientUpdater&) = delete;
  ~BraveVpnClientUpdater() override;

  void Register();
  base::FilePath GetExecutablePath() const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

  bool IsRegistered() const { return registered_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void Cleanup();

 protected:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  // BraveComponent::ComponentObserver
  void OnEvent(Events event, const std::string& id) override;

 private:
  //friend class ::BraveVpnClientUpdaterTest;
  static std::string g_vpn_client_component_name_;
  static std::string g_vpn_client_component_id_;
  static std::string g_vpn_client_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  void SetExecutablePath(const base::FilePath& path);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;
  base::FilePath user_data_dir_;
  base::FilePath executable_path_;
  base::ObserverList<Observer> observers_;
  base::ScopedObservation<BraveComponent, BraveComponent::ComponentObserver>
      updater_observer_{this};

  base::WeakPtrFactory<BraveVpnClientUpdater> weak_ptr_factory_;
};

// Creates the BraveVpnClientUpdater
std::unique_ptr<BraveVpnClientUpdater> BraveVpnClientUpdaterFactory(
    BraveComponent::Delegate* delegate,
    const base::FilePath& user_data_dir);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_CLIENT_UPDATER_H_
