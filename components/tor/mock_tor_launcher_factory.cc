/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/mock_tor_launcher_factory.h"

#include "brave/components/tor/tor_launcher_observer.h"

// static
MockTorLauncherFactory& MockTorLauncherFactory::GetInstance() {
  static base::NoDestructor<MockTorLauncherFactory> instance;
  return *instance;
}

MockTorLauncherFactory::MockTorLauncherFactory() = default;
MockTorLauncherFactory::~MockTorLauncherFactory() = default;

void MockTorLauncherFactory::NotifyObservers(
    base::RepeatingCallback<void(TorLauncherObserver&)> notify) {
  for (auto& observer : observers_) {
    notify.Run(observer);
  }
}
