/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_MOCK_TOR_LAUNCHER_FACTORY_H_
#define BRAVE_COMPONENTS_TOR_MOCK_TOR_LAUNCHER_FACTORY_H_

#include <string>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "testing/gmock/include/gmock/gmock.h"

class MockTorLauncherFactory : public TorLauncherFactory {
 public:
  static MockTorLauncherFactory& GetInstance();

  MOCK_METHOD(void, Init, (), (override));
  MOCK_METHOD(void,
              LaunchTorProcess,
              (const tor::mojom::TorConfig& config),
              (override));
  MOCK_METHOD(void, KillTorProcess, (), (override));
  MOCK_METHOD(int64_t, GetTorPid, (), (const override));
  MOCK_METHOD(bool, IsTorConnected, (), (const override));
  MOCK_METHOD(std::string, GetTorProxyURI, (), (const override));

  void NotifyObservers(
      base::RepeatingCallback<void(TorLauncherObserver&)> notify);

 private:
  friend class base::NoDestructor<MockTorLauncherFactory>;

  MockTorLauncherFactory();
  ~MockTorLauncherFactory() override;

  MockTorLauncherFactory(const MockTorLauncherFactory&) = delete;
  MockTorLauncherFactory& operator=(const MockTorLauncherFactory&) = delete;
};

#endif  // BRAVE_COMPONENTS_TOR_MOCK_TOR_LAUNCHER_FACTORY_H_
