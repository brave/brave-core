/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_H_
#define BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_H_

#include <memory>
#include <string>

#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}

namespace content {
class WebContents;
}

namespace net {
class ProxyConfigService;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class TorLauncherFactory;
class PrefRegistrySimple;

namespace tor {

class TorProfileService : public KeyedService {
 public:
  TorProfileService();
  TorProfileService(const TorProfileService&) = delete;
  TorProfileService& operator=(const TorProfileService&) = delete;
  ~TorProfileService() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  virtual void RegisterTorClientUpdater() = 0;
  virtual void UnregisterTorClientUpdater() = 0;
  virtual void SetNewTorCircuit(content::WebContents* web_contents) = 0;
  virtual std::unique_ptr<net::ProxyConfigService>
      CreateProxyConfigService() = 0;
  virtual bool IsTorConnected() = 0;
  virtual void KillTor() = 0;
  virtual void SetTorLauncherFactoryForTest(TorLauncherFactory* factory) {}
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_PROFILE_SERVICE_H_
