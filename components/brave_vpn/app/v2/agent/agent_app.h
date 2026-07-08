/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_AGENT_APP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_AGENT_APP_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/single_thread_task_runner.h"

namespace brave_vpn::v2 {

class ShutdownHandlers;

// The AgentApp class encapsulates the main application logic for the Brave VPN
// agent process. It owns the main thread's task executor and run loop, and
// funnels external termination requests (POSIX signals, Windows console control
// events, and eventually Mojo Shutdown()/disconnect) into a single graceful
// shutdown path.
class AgentApp final {
 public:
  AgentApp();
  ~AgentApp();

  AgentApp(const AgentApp&) = delete;
  AgentApp& operator=(const AgentApp&) = delete;

  // Runs the main event loop of the application. The return value is the exit
  // code of the application. Must be called exactly once, on the main thread.
  int Run();

  // Requests a graceful shutdown. Idempotent and safe to call from any thread
  // once Run() has started.
  void Shutdown();

 private:
  // Runs exclusively on the main thread to perform cleanup and quit the run
  // loop.
  void ExecuteShutdown();

  scoped_refptr<base::SingleThreadTaskRunner> main_runner_;
  std::unique_ptr<ShutdownHandlers> shutdown_handlers_;
  base::OnceClosure quit_closure_;

  bool is_shutting_down_ = false;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_AGENT_APP_H_
