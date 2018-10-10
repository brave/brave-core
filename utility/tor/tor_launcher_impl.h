/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_TOR_TOR_LAUNCHER_IMPL_H_
#define BRAVE_UTILITY_TOR_TOR_LAUNCHER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/process/process.h"
#include "brave/common/tor/tor_launcher.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace tor {

class TorLauncherImpl : public tor::mojom::TorLauncher {
 public:
  explicit TorLauncherImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~TorLauncherImpl() override;

  // tor::mojom::TorLauncher
  void Launch(const TorConfig& config,
              LaunchCallback callback) override;
  void SetCrashHandler(SetCrashHandlerCallback callback) override;
  void ReLaunch(const TorConfig& config,
              ReLaunchCallback callback) override;

 private:
  void MonitorChild();

  SetCrashHandlerCallback crash_handler_callback_;
  std::unique_ptr<base::Thread> child_monitor_thread_;
  base::Process tor_process_;
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(TorLauncherImpl);
};

}  // namespace tor

#endif  // BRAVE_UTILITY_TOR_TOR_LAUNCHER_IMPL_H_
