/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_IMPL_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "base/sequence_checker.h"
#include "base/threading/thread.h"
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace tor {

class TorLauncherImpl : public tor::mojom::TorLauncher {
 public:
  explicit TorLauncherImpl(
      mojo::PendingReceiver<tor::mojom::TorLauncher> receiver);
  ~TorLauncherImpl() override;

  // tor::mojom::TorLauncher
  void Shutdown() override;
  void Launch(mojom::TorConfigPtr config,
              LaunchCallback callback) override;
  void SetCrashHandler(SetCrashHandlerCallback callback) override;

 private:
  void OnChildCrash(base::ProcessId pid);
  void Cleanup();

  SetCrashHandlerCallback crash_handler_callback_;
  std::unique_ptr<base::Thread> child_monitor_thread_;
  base::Process tor_process_;
  mojo::Receiver<tor::mojom::TorLauncher> receiver_;
  bool in_shutdown_ = false;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<TorLauncherImpl> weak_ptr_factory_{this};

  TorLauncherImpl(const TorLauncherImpl&) = delete;
  TorLauncherImpl& operator=(const TorLauncherImpl&) = delete;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_IMPL_H_
