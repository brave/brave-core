/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_H_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/common/tor/tor_common.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace net {
class ProxyResolutionService;
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

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  virtual void LaunchTor(const TorConfig&) = 0;
  virtual void ReLaunchTor(const TorConfig&) = 0;
  virtual void SetNewTorCircuit(const GURL& request_url,
                                const base::Closure&) = 0;
  virtual const TorConfig& GetTorConfig() = 0;
  virtual int64_t GetTorPid() = 0;

  virtual int SetProxy(net::ProxyResolutionService*,
                       const GURL& request_url,
                       bool new_circuit) = 0;

  void AddObserver(TorLauncherServiceObserver* observer);
  void RemoveObserver(TorLauncherServiceObserver* observer);

 protected:
  base::ObserverList<TorLauncherServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TorProfileService);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_H_
