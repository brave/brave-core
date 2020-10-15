/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
// TODO(darkdh): move to components/tor in a following commit
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_profile_service.h"
#include "net/proxy_resolution/proxy_info.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace net {
class ProxyConfigService;
class ProxyConfigServiceTor;
}

using extensions::BraveTorClientUpdater;

namespace tor {

using NewTorCircuitCallback = base::OnceCallback<void(
    const base::Optional<net::ProxyInfo>& proxy_info)>;

class TorProfileServiceImpl : public TorProfileService,
                              public BraveTorClientUpdater::Observer {
 public:
  TorProfileServiceImpl(content::BrowserContext* context,
                        BraveTorClientUpdater* tor_client_updater,
                        const base::FilePath& user_data_dir);
  ~TorProfileServiceImpl() override;

  // TorProfileService:
  void RegisterTorClientUpdater() override;
  void UnregisterTorClientUpdater() override;
  void SetNewTorCircuit(content::WebContents* web_contents) override;
  std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService() override;
  bool IsTorConnected() override;
  void SetTorLaunchedForTest() override;

  void KillTor();

  // For internal observer
  void NotifyTorLauncherCrashed();
  void NotifyTorCrashed(int64_t pid);
  void NotifyTorLaunched(bool result, int64_t pid);
  void NotifyTorNewProxyURI(const std::string& uri);
  void NotifyTorCircuitEstablished(bool result);
  void NotifyTorInitializing(const std::string& percentage);

 private:
  void LaunchTor();

  base::FilePath GetTorExecutablePath();
  base::FilePath GetTorDataPath();
  base::FilePath GetTorWatchPath();

  // BraveTorClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;

  bool is_tor_launched_for_test_ = false;
  content::BrowserContext* context_ = nullptr;
  BraveTorClientUpdater* tor_client_updater_ = nullptr;
  base::FilePath user_data_dir_;
  TorLauncherFactory* tor_launcher_factory_;  // Singleton
  net::ProxyConfigServiceTor* proxy_config_service_;  // NOT OWNED
  base::WeakPtrFactory<TorProfileServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceImpl);
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
