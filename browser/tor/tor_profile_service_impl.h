/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_

#include "brave/browser/tor/tor_profile_service.h"

#include "brave/browser/tor/tor_launcher_factory.h"
#include "brave/browser/tor/tor_proxy_config_service.h"

class Profile;

namespace tor {

class TorProfileServiceImpl : public TorProfileService {
 public:
  TorProfileServiceImpl(Profile* profile);
  ~TorProfileServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  // TorProfileService:
  void LaunchTor(const TorConfig&) override;
  void ReLaunchTor(const TorConfig&) override;
  void SetNewTorCircuit(const GURL&) override;
  const TorConfig& GetTorConfig() override;
  int64_t GetTorPid() override;

  void SetProxy(net::ProxyResolutionService*, const GURL&,
                        bool /* new circuit */) override;

  void KillTor();
 private:
  Profile* profile_;  // NOT OWNED
  TorLauncherFactory* tor_launcher_factory_; // Singleton
  TorProxyConfigService::TorProxyMap tor_proxy_map_;
  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceImpl);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_
