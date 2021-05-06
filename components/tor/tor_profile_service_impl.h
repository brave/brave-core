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
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
#include "net/proxy_resolution/proxy_info.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace net {
class ProxyConfigService;
class ProxyConfigServiceTor;
}

namespace tor {

using NewTorCircuitCallback = base::OnceCallback<void(
    const base::Optional<net::ProxyInfo>& proxy_info)>;

class TorProfileServiceImpl : public TorProfileService,
                              public BraveTorClientUpdater::Observer,
                              public TorLauncherObserver {
 public:
  TorProfileServiceImpl(content::BrowserContext* context,
                        BraveTorClientUpdater* tor_client_updater);
  ~TorProfileServiceImpl() override;

  // TorProfileService:
  void RegisterTorClientUpdater() override;
  void UnregisterTorClientUpdater() override;
  void SetNewTorCircuit(content::WebContents* web_contents) override;
  std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService() override;
  bool IsTorConnected() override;
  void KillTor() override;
  void SetTorLauncherFactoryForTest(TorLauncherFactory* factory) override;

  // TorLauncherObserver:
  void OnTorNewProxyURI(const std::string& uri) override;

 private:
  void LaunchTor();

  base::FilePath GetTorExecutablePath() const;
  base::FilePath GetTorDataPath() const;
  base::FilePath GetTorWatchPath() const;

  // BraveTorClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;

  content::BrowserContext* context_ = nullptr;
  BraveTorClientUpdater* tor_client_updater_ = nullptr;
  TorLauncherFactory* tor_launcher_factory_;  // Singleton
  net::ProxyConfigServiceTor* proxy_config_service_;  // NOT OWNED
  base::WeakPtrFactory<TorProfileServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceImpl);
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
