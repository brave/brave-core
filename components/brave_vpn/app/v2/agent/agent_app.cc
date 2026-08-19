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
#include "base/strings/utf_ostream_operators.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_registry.h"
#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"
#include "brave/components/brave_vpn/common/v2/agent_utils.h"
#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"

namespace brave_vpn::v2 {
namespace {
constexpr char kMojoIpcThreadName[] = "MojoIPC";
}  // namespace

// Unretained is safe: the shutdown handlers may invoke this callback from a
// background thread, but only while the process (and hence |this|, which
// lives for main()'s duration) is alive.
AgentApp::AgentApp()
    : shutdown_handlers_(std::make_unique<ShutdownHandlers>(
          base::BindOnce(&AgentApp::Shutdown, base::Unretained(this)))) {}

AgentApp::~AgentApp() = default;

int AgentApp::Run() {
  const auto server_name = GetAgentServerName();
  if (!server_name) {
    return 1;  // Reason already logged. Non-zero so systemd/launchd sees it.
  }

  VLOG(1) << "Hello from the Brave VPN Agent: server name = " << *server_name;

  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::IO);
  main_runner_ = base::SingleThreadTaskRunner::GetCurrentDefault();

  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();

  // Once installed, the handlers may invoke Shutdown() from a background thread
  // at any moment, which requires |main_runner_| and |quit_closure_| to already
  // be in place.
  if (!shutdown_handlers_->Install()) {
    VLOG(1) << "Shutdown handlers not installed";
  }

  {
    // Initialize Mojo on a separate thread.
    base::Thread ipc_thread(kMojoIpcThreadName);
    CHECK(ipc_thread.StartWithOptions(
        base::Thread::Options(base::MessagePumpType::IO, 0)));

    mojo::core::Init(mojo::core::Configuration{.is_broker_process = true});
    mojo::core::ScopedIPCSupport ipc_support(
        ipc_thread.task_runner(),
        mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

    // Initialize the browser registry, which will manage the lifetime of
    // client browser instances and their associated Mojo connections.
    BrowserRegistry browser_registry{*server_name};

    run_loop.Run();
  }

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
  if (quit_closure_) {
    VLOG(1) << "Shutting down Brave VPN Agent";
    std::move(quit_closure_).Run();
  }
}

}  // namespace brave_vpn::v2
