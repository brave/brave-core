/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
#define BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_

#include <string>

#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "brave/common/tor/tor_common.h"
#include "brave/common/tor/tor_control.h"
#include "brave/common/tor/tor_control_observer.h"
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace tor {
class TorProfileServiceImpl;
}

class TorLauncherFactory : public tor::TorControlObserver {
 public:
  static TorLauncherFactory* GetInstance();

  void Init();
  void LaunchTorProcess(const tor::TorConfig& config);
  void ReLaunchTorProcess(const tor::TorConfig& config);
  void KillTorProcess();
  const tor::TorConfig& GetTorConfig() const { return config_; }
  int64_t GetTorPid() const { return tor_pid_; }

  void AddObserver(tor::TorProfileServiceImpl* serice);
  void RemoveObserver(tor::TorProfileServiceImpl* service);

  // tor::TorControlObserver
  void OnTorControlReady() override;
  void OnTorClosed() override;
  void OnTorEvent(
      tor::TorControlEvent event,
      const std::string& initial,
      const std::map<std::string, std::string>& extra)
    override;
  void OnTorRawCmd(const std::string& cmd) override;
  void OnTorRawAsync(const std::string& status,
                     const std::string& line)
    override;
  void OnTorRawMid(const std::string& status,
                   const std::string& line)
    override;
  void OnTorRawEnd(const std::string& status,
                   const std::string& line)
    override;

 private:
  friend struct base::DefaultSingletonTraits<TorLauncherFactory>;

  TorLauncherFactory();
  ~TorLauncherFactory() final;

  bool SetConfig(const tor::TorConfig& config);

  void OnTorLauncherCrashed();
  void OnTorCrashed(int64_t pid);
  void OnTorLaunched(bool result, int64_t pid);

  void GotVersion(bool error, const std::string& version);
  void Loggerific(
      bool error, const std::string& status, const std::string& reply);

  bool is_starting_;

  mojo::Remote<tor::mojom::TorLauncher> tor_launcher_;

  int64_t tor_pid_;

  tor::TorConfig config_;

  base::ObserverList<tor::TorProfileServiceImpl> observers_;

  scoped_refptr<tor::TorControl> control_;

  DISALLOW_COPY_AND_ASSIGN(TorLauncherFactory);
};

// Use this in tests to avoid the actual launch of the Tor process.
class ScopedTorLaunchPreventerForTest {
 public:
  ScopedTorLaunchPreventerForTest();
  ~ScopedTorLaunchPreventerForTest();
};

#endif  // BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
