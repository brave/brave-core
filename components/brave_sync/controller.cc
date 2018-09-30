/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/controller.h"
#include "brave/components/brave_sync/controller_observer.h"

namespace brave_sync {

  Controller::Controller() {}

  Controller::~Controller() {}

  void Controller::AddObserver(ControllerObserver* observer) {
    observers_.AddObserver(observer);
  }

  void Controller::RemoveObserver(ControllerObserver* observer) {
    observers_.RemoveObserver(observer);
  }
} // brave_sync
