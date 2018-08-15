/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service.h"

#include "brave/browser/tor/tor_launcher_service_observer.h"

namespace tor {

TorProfileService::TorProfileService() {
}

TorProfileService::~TorProfileService() {
}

void TorProfileService::AddObserver(TorLauncherServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void TorProfileService::RemoveObserver(TorLauncherServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace tor
