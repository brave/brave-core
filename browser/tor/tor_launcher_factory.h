/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
#define BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_

#include <string>

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "brave/common/tor/tor_common.h"
#include "brave/common/tor/tor_launcher.mojom.h"

class TorLauncherFactory {
 public:
  static TorLauncherFactory* GetInstance();

  void LaunchTorProcess(const tor::TorConfig& config);
  void ReLaunchTorProcess(const tor::TorConfig& config);
  void KillTorProcess();
  const tor::TorConfig& GetTorConfig() const { return config_; }
  int64_t GetTorPid() const { return tor_pid_; }

 private:
  friend struct base::DefaultSingletonTraits<TorLauncherFactory>;

  TorLauncherFactory();
  ~TorLauncherFactory();

  bool SetConfig(const tor::TorConfig& config);

  void OnTorLauncherCrashed();
  void OnTorCrashed(int64_t pid);
  void OnTorLaunched(bool result, int64_t pid);

  tor::mojom::TorLauncherPtr tor_launcher_;

  int64_t tor_pid_;

  tor::TorConfig config_;

  DISALLOW_COPY_AND_ASSIGN(TorLauncherFactory);
};

#endif  // BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
