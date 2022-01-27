/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_LAUNCHER_FACTORY_H_
#define BRAVE_COMPONENTS_TOR_TOR_LAUNCHER_FACTORY_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "brave/components/tor/tor_control.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace base {
class SequencedTaskRunner;
struct OnTaskRunnerDeleter;
}  // namespace base

class MockTorLauncherFactory;
class TorLauncherObserver;

class TorLauncherFactory : public tor::TorControl::Delegate {
 public:
  using GetLogCallback = base::OnceCallback<void(bool, const std::string&)>;

  TorLauncherFactory(const TorLauncherFactory&) = delete;
  TorLauncherFactory& operator=(const TorLauncherFactory&) = delete;

  static TorLauncherFactory* GetInstance();

  virtual void Init();
  virtual void LaunchTorProcess(const tor::mojom::TorConfig& config);
  virtual void KillTorProcess();
  virtual int64_t GetTorPid() const;
  virtual bool IsTorConnected() const;
  virtual std::string GetTorProxyURI() const;
  virtual std::string GetTorVersion() const;
  virtual void GetTorLog(GetLogCallback);

  void AddObserver(TorLauncherObserver* observer);
  void RemoveObserver(TorLauncherObserver* observer);

  // tor::TorControl::Delegate
  void OnTorControlReady() override;
  void OnTorControlClosed(bool was_running) override;
  void OnTorEvent(tor::TorControlEvent event,
                  const std::string& initial,
                  const std::map<std::string, std::string>& extra) override;
  void OnTorRawCmd(const std::string& cmd) override;
  void OnTorRawAsync(const std::string& status,
                     const std::string& line) override;
  void OnTorRawMid(const std::string& status, const std::string& line) override;
  void OnTorRawEnd(const std::string& status, const std::string& line) override;

 private:
  friend struct base::DefaultSingletonTraits<TorLauncherFactory>;
  friend class MockTorLauncherFactory;

  TorLauncherFactory();
  ~TorLauncherFactory() override;

  void OnTorControlPrerequisitesReady(int64_t pid,
                                      bool ready,
                                      std::vector<uint8_t> cookie,
                                      int port);

  void OnTorLauncherCrashed();
  void OnTorCrashed(int64_t pid);
  void OnTorLaunched(bool result, int64_t pid);

  void GotVersion(bool error, const std::string& version);
  void GotSOCKSListeners(bool error, const std::vector<std::string>& listeners);
  void GotCircuitEstablished(bool error, bool established);

  void LaunchTorInternal();
  void RelaunchTor();
  void DelayedRelaunchTor();

  bool is_starting_;
  bool is_connected_;

  mojo::Remote<tor::mojom::TorLauncher> tor_launcher_;

  std::string tor_proxy_uri_;
  std::string tor_version_;

  std::string tor_log_;

  int64_t tor_pid_;

  tor::mojom::TorConfig config_;

  base::ObserverList<TorLauncherObserver> observers_;

  std::unique_ptr<tor::TorControl, base::OnTaskRunnerDeleter> control_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<TorLauncherFactory> weak_ptr_factory_;
};

#endif  // BRAVE_COMPONENTS_TOR_TOR_LAUNCHER_FACTORY_H_
