/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_impl.h"

#include "chrome/browser/profiles/profile.h"

namespace tor {

TorProfileServiceImpl::TorProfileServiceImpl(Profile* profile) :
    profile_(profile) {
  //TODO: remove it
  LOG(ERROR) << profile_;
}

TorProfileServiceImpl::~TorProfileServiceImpl() {
}

void TorProfileServiceImpl::Shutdown() {
  TorProfileService::Shutdown();
}
void TorProfileServiceImpl::LaunchTor(const TorConfig& config,
                                      const TorLaunchCallback& callback) {}
void TorProfileServiceImpl::ReLaunchTor(const TorLaunchCallback& callback) {}
void TorProfileServiceImpl::SetNewTorCircuit(const GURL& url) {}
bool TorProfileServiceImpl::UpdateNewTorConfig(const TorConfig& config) { return true;}
int64_t TorProfileServiceImpl::GetTorPid() {return 0;}

}  // namespace tor
