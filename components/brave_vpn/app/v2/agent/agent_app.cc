/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/agent_app.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"

namespace brave_vpn::v2 {

AgentApp::AgentApp()
    : shutdown_handlers_(std::make_unique<ShutdownHandlers>(
          base::BindRepeating(&AgentApp::Shutdown, base::Unretained(this)))) {}

AgentApp::~AgentApp() = default;

int AgentApp::Run() {
  VLOG(1) << "Hello from the Brave VPN Agent!";

  base::SingleThreadTaskExecutor main_task_executor(
      base::MessagePumpType::DEFAULT);
  main_runner_ = base::SingleThreadTaskRunner::GetCurrentDefault();

  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();

  // Once installed, the handlers may invoke Shutdown() from a background thread
  // at any moment, which requires |main_runner_| and |quit_closure_| to already
  // be in place.
  if (!shutdown_handlers_->Install()) {
    VLOG(1) << "Shutdown handlers not installed";
  }
  run_loop.Run();

  shutdown_handlers_->SignalShutdownComplete();
  return 0;
}

void AgentApp::Shutdown() {
  CHECK(main_runner_) << "Shutdown() called before Run()";
  if (main_runner_->RunsTasksInCurrentSequence()) {
    ExecuteShutdown();
  } else {
    main_runner_->PostTask(FROM_HERE, base::BindOnce(&AgentApp::ExecuteShutdown,
                                                     base::Unretained(this)));
  }
}

void AgentApp::ExecuteShutdown() {
  if (is_shutting_down_) {
    return;
  }
  is_shutting_down_ = true;
  VLOG(1) << "Shutting down Brave VPN Agent";
  if (quit_closure_) {
    std::move(quit_closure_).Run();
  }
}

}  // namespace brave_vpn::v2
