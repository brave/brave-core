/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/proxy_resolution/proxy_config.h"
#include "url/gurl.h"

namespace tor {

struct TorConfig {
  base::FilePath binary_path;
  net::ProxyConfig proxy_config;
};

using TorLaunchCallback =
    base::Callback<void(bool /* result */, int64_t /* pid */)>;

class TorLauncherServiceObserver;

class TorProfileService : public KeyedService {
 public:
  TorProfileService();
  ~TorProfileService() override;

  virtual void LaunchTor(const TorConfig&, const TorLaunchCallback&) = 0;
  virtual void ReLaunchTor(const TorLaunchCallback&) = 0;
  virtual void SetNewTorCircuit(const GURL&) = 0;
  virtual bool UpdateNewTorConfig(const TorConfig&) = 0;
  virtual int64_t GetTorPid() = 0;

  void AddObserver(TorLauncherServiceObserver* observer);
  void RemoveObserver(TorLauncherServiceObserver* observer);

 protected:
  base::ObserverList<TorLauncherServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TorProfileService);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_
