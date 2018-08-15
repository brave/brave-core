/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_

#include "brave/browser/tor/tor_profile_service.h"

class Profile;

namespace tor {

class TorProfileServiceImpl : public TorProfileService {
 public:
  TorProfileServiceImpl(Profile* profile);
  ~TorProfileServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  // TorProfileService:
  void LaunchTor(const TorConfig&, const TorLaunchCallback&) override;
  void ReLaunchTor(const TorLaunchCallback&) override;
  void SetNewTorCircuit(const GURL&) override;
  bool UpdateNewTorConfig(const TorConfig&) override;
  int64_t GetTorPid() override;
 private:
  Profile* profile_;  // NOT OWNED
  // TODO: tor proxy map
  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceImpl);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_IMPL_
