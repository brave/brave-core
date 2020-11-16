/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_LAUNCHER_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_TOR_TOR_LAUNCHER_SERVICE_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace tor {

class TorLauncherServiceObserver : public base::CheckedObserver {
 public:
  ~TorLauncherServiceObserver() override {}

  virtual void OnTorLauncherCrashed() {}
  virtual void OnTorCrashed(int64_t pid) {}
  virtual void OnTorLaunched(bool result, int64_t pid) {}
  virtual void OnTorCircuitEstablished(bool result) {}
  virtual void OnTorInitializing(const std::string& percentage) {}
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_LAUNCHER_SERVICE_OBSERVER_H_
