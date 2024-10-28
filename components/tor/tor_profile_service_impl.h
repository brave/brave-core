/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "net/proxy_resolution/proxy_info.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace net {
class ProxyConfigService;
class ProxyConfigServiceTor;
}  // namespace net

namespace tor {

using NewTorCircuitCallback =
    base::OnceCallback<void(const std::optional<net::ProxyInfo>& proxy_info)>;

class TorProfileServiceImpl
    : public TorProfileService,
      public BraveTorClientUpdater::Observer,
      public BraveTorPluggableTransportUpdater::Observer,
      public TorLauncherObserver {
 public:
  TorProfileServiceImpl(
      content::BrowserContext* original_context,
      content::BrowserContext* context,
      PrefService* local_state,
      BraveTorClientUpdater* tor_client_updater,
      BraveTorPluggableTransportUpdater* to_pluggable_transport_updater);
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
  void OnTorControlReady() override;
  void OnTorNewProxyURI(const std::string& uri) override;

 private:
  void LaunchTor();

  // BraveTorClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;

  // BraveTorPluggableTransportUpdater::Observer
  void OnPluggableTransportReady(bool success) override;

  void OnBridgesConfigChanged();

  void OnBuiltinBridgesResponse(const base::Value::Dict& bridges);

  raw_ptr<content::BrowserContext> context_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<BraveTorClientUpdater> tor_client_updater_ = nullptr;
  raw_ptr<BraveTorPluggableTransportUpdater> tor_pluggable_transport_updater_ =
      nullptr;
  raw_ptr<TorLauncherFactory> tor_launcher_factory_ = nullptr;  // Singleton
  raw_ptr<net::ProxyConfigServiceTor, DanglingUntriaged> proxy_config_service_ =
      nullptr;  // NOT OWNED
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<class BuiltinBridgesRequest> builtin_bridges_request_;
  base::WeakPtrFactory<TorProfileServiceImpl> weak_ptr_factory_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_IMPL_H_
