/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_LAUNCHER_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_TOR_TOR_LAUNCHER_SERVICE_OBSERVER_H_

namespace tor {

class TorLauncherServiceObserver {
 public:
  virtual ~TorLauncherServiceObserver() {}

  virtual void OnTorCrash(int64_t pid) {};
  virtual void OnTorLauncherCrash() {};
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_LAUNCHER_SERVICE_OBSERVER_H_
