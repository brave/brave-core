/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_H_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/common/tor/tor_common.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace net {
class ProxyConfigService;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefRegistrySimple;

namespace tor {

class TorLauncherServiceObserver;

class TorProfileService : public KeyedService {
 public:
  TorProfileService();
  ~TorProfileService() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  virtual void SetNewTorCircuit(content::WebContents* web_contents) = 0;
  virtual std::unique_ptr<net::ProxyConfigService>
      CreateProxyConfigService() = 0;
  void AddObserver(TorLauncherServiceObserver* observer);
  void RemoveObserver(TorLauncherServiceObserver* observer);

 protected:
  std::string GetTorProxyURI();
  base::FilePath GetTorExecutablePath();
  base::ObserverList<TorLauncherServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TorProfileService);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_H_
