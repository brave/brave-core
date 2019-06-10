/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_H_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_H_

#include "brave/browser/tor/tor_profile_service.h"

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "brave/browser/tor/tor_launcher_factory.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/proxy_lookup_client.mojom.h"

class Profile;

namespace network {
namespace mojom {
class NetworkContext;
}
}  // namespace network

namespace tor {

class TorProfileServiceImpl : public TorProfileService,
                              public base::CheckedObserver,
                              public network::mojom::ProxyLookupClient {
 public:
  explicit TorProfileServiceImpl(Profile* profile);
  ~TorProfileServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  // TorProfileService:
  void LaunchTor(const TorConfig&) override;
  void ReLaunchTor(const TorConfig&) override;
  void SetNewTorCircuit(const GURL& request_url,
                        NewTorCircuitCallback) override;
  const TorConfig& GetTorConfig() override;
  int64_t GetTorPid() override;

  void KillTor();

  // For internal observer
  void NotifyTorLauncherCrashed();
  void NotifyTorCrashed(int64_t pid);
  void NotifyTorLaunched(bool result, int64_t pid);

  // network::mojom::ProxyLookupClient:
  void OnProxyLookupComplete(
      int32_t net_error,
      const base::Optional<net::ProxyInfo>& proxy_info) override;

 private:
  void OnSetNewTorCircuitComplete(bool success);

  Profile* profile_;  // NOT OWNED
  TorLauncherFactory* tor_launcher_factory_;  // Singleton
  NewTorCircuitCallback tor_circuit_callback_;
  mojo::Binding<network::mojom::ProxyLookupClient> binding_;
  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceImpl);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_H_
