/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
#include "net/proxy_resolution/proxy_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace net {
class ProxyConfigService;
class ProxyConfigServiceTor;
}

namespace tor {

using NewTorCircuitCallback =
    base::OnceCallback<void(const absl::optional<net::ProxyInfo>& proxy_info)>;

class TorProfileServiceImpl : public TorProfileService,
                              public BraveTorClientUpdater::Observer,
                              public TorLauncherObserver {
 public:
  TorProfileServiceImpl(content::BrowserContext* context,
                        BraveTorClientUpdater* tor_client_updater);
  TorProfileServiceImpl(const TorProfileServiceImpl&) = delete;
  TorProfileServiceImpl& operator=(const TorProfileServiceImpl&) = delete;
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
  base::FilePath GetTorrcPath() const;
  base::FilePath GetTorDataPath() const;
  base::FilePath GetTorWatchPath() const;

  // BraveTorClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;

  content::BrowserContext* context_ = nullptr;
  BraveTorClientUpdater* tor_client_updater_ = nullptr;
  raw_ptr<TorLauncherFactory> tor_launcher_factory_ = nullptr;  // Singleton
  raw_ptr<net::ProxyConfigServiceTor> proxy_config_service_ =
      nullptr;  // NOT OWNED
  base::WeakPtrFactory<TorProfileServiceImpl> weak_ptr_factory_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
