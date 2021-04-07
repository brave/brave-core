/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/process/process.h"
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
  void MonitorChild();
  void Cleanup();

  SetCrashHandlerCallback crash_handler_callback_;
  std::unique_ptr<base::Thread> child_monitor_thread_;
  scoped_refptr<base::SequencedTaskRunner> main_task_runner_;
  base::Process tor_process_;
  mojo::Receiver<tor::mojom::TorLauncher> receiver_;
  bool in_shutdown_ = false;

  DISALLOW_COPY_AND_ASSIGN(TorLauncherImpl);
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_IMPL_H_
