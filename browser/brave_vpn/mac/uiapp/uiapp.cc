// Copyright 2024 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <signal.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/memory/weak_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server_client_util.h"
#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/invitation.h"

#include "brave/browser/brave_vpn/mac/ipc/poc.mojom.h"

namespace {

constexpr char kAgentServerNamePrefix[] = "poc_agent_ipc_";
constexpr char kUIAppAuthToken[] = "poc-uiapp-secret-do-not-use-in-production";

constexpr base::TimeDelta kReconnectDelay = base::Seconds(1);
constexpr base::TimeDelta kTestCycleInterval = base::Seconds(3);

// ── Self-pipe for signal handling ────────────────────────────────────────────
int g_signal_pipe[2] = {-1, -1};

void SignalHandler(int /*sig*/) {
  char byte = 1;
  write(g_signal_pipe[1], &byte, 1);  // NOLINT(cert-err33-c)
}

// ============================================================================
// UIAppServiceImpl — exposed to agent.
// ============================================================================

class UIAppServiceImpl : public poc::mojom::UIAppService {
 public:
  void TestUIApp(TestUIAppCallback callback) override {
    LOG(INFO) << "[UIApp] TestUIApp() received. Sending response.";
    std::move(callback).Run("Hello from uiapp");
  }
};

// ============================================================================
// UIAppApp
// ============================================================================

class UIAppApp {
 public:
  UIAppApp(const std::string& agent_server_name)
      : agent_server_name_(agent_server_name),
        uiapp_receiver_(&uiapp_impl_) {}

  void Start() {
    LOG(INFO) << "[UIApp] Starting.";
    TryConnect();
  }

 private:
  void TryConnect() {
    Reset();

    LOG(INFO) << "[UIApp] Connecting to \"" << agent_server_name_ << "\"...";

    mojo::PlatformChannelEndpoint endpoint =
        named_mojo_ipc_server::ConnectToServer(agent_server_name_);

    if (!endpoint.is_valid()) {
      LOG(INFO) << "[UIApp] Agent not reachable. Retrying in "
                << kReconnectDelay << ".";
      ScheduleReconnect();
      return;
    }

    // uiapp is a broker, so use AcceptIsolated rather than Accept.
    mojo::ScopedMessagePipeHandle pipe =
        mojo::IncomingInvitation::AcceptIsolated(std::move(endpoint));

    if (!pipe.is_valid()) {
      LOG(ERROR) << "[UIApp] AcceptIsolated returned invalid pipe.";
      ScheduleReconnect();
      return;
    }

    agent_service_.Bind(
        mojo::PendingRemote<poc::mojom::AgentUserService>(std::move(pipe), 0));
    agent_service_.set_disconnect_handler(
        base::BindOnce(&UIAppApp::OnAgentDisconnected,
                       weak_factory_.GetWeakPtr()));

    mojo::PendingRemote<poc::mojom::UIAppService> uiapp_remote;
    uiapp_receiver_.Bind(uiapp_remote.InitWithNewPipeAndPassReceiver());
    uiapp_receiver_.set_disconnect_handler(
        base::BindOnce(&UIAppApp::OnUIAppReceiverDisconnected,
                       weak_factory_.GetWeakPtr()));

    LOG(INFO) << "[UIApp] Sending Authenticate().";
    agent_service_->Authenticate(
        kUIAppAuthToken, std::move(uiapp_remote),
        base::BindOnce(&UIAppApp::OnAuthenticated,
                       weak_factory_.GetWeakPtr()));
  }

  void OnAuthenticated(bool accepted) {
    if (!accepted) {
      LOG(ERROR) << "[UIApp] Authentication rejected. Retrying in "
                 << kReconnectDelay << ".";
      ScheduleReconnect();
      return;
    }
    LOG(INFO) << "[UIApp] Authenticated. Starting test cycle.";
    CallTestAgentUser();
  }

  void CallTestAgentUser() {
    if (!agent_service_.is_bound() || !agent_service_.is_connected()) {
      LOG(WARNING) << "[UIApp] Agent gone; stopping cycle.";
      return;
    }
    LOG(INFO) << "[UIApp] Calling TestAgentUser().";
    agent_service_->TestAgentUser(
        base::BindOnce(&UIAppApp::OnTestAgentUserResponse,
                       weak_factory_.GetWeakPtr()));
  }

  void OnTestAgentUserResponse(const std::string& response) {
    LOG(INFO) << "[UIApp] TestAgentUser() returned: \"" << response << "\".";
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&UIAppApp::CallTestAgentUser,
                       weak_factory_.GetWeakPtr()),
        kTestCycleInterval);
  }

  void OnAgentDisconnected() {
    LOG(INFO) << "[UIApp] Agent disconnected. Reconnecting in "
              << kReconnectDelay << ".";
    ScheduleReconnect();
  }

  void OnUIAppReceiverDisconnected() {
    LOG(INFO) << "[UIApp] UIAppService receiver disconnected.";
    // OnAgentDisconnected() will follow and drive the reconnect.
  }

  void Reset() {
    agent_service_.reset();
    if (uiapp_receiver_.is_bound()) uiapp_receiver_.reset();
  }

  void ScheduleReconnect() {
    weak_factory_.InvalidateWeakPtrs();
    Reset();
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&UIAppApp::TryConnect, weak_factory_.GetWeakPtr()),
        kReconnectDelay);
  }

  const std::string agent_server_name_;
  mojo::Remote<poc::mojom::AgentUserService> agent_service_;
  UIAppServiceImpl uiapp_impl_;
  mojo::Receiver<poc::mojom::UIAppService> uiapp_receiver_;
  base::WeakPtrFactory<UIAppApp> weak_factory_{this};
};

}  // namespace

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);

  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  logging::InitLogging(settings);
  logging::SetMinLogLevel(logging::LOGGING_INFO);

  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("poc_uiapp");

  base::SingleThreadTaskExecutor main_executor(base::MessagePumpType::IO);
  base::FileDescriptorWatcher fd_watcher(main_executor.task_runner());

  // uiapp is a broker (mirrors real Chromium browser process).
  mojo::core::Configuration config;
  config.is_broker_process = true;
  mojo::core::Init(config);

  base::Thread ipc_thread("MojoIPC");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessagePumpType::IO, 0));
  mojo::core::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  // ── Signal handling ────────────────────────────────────────────────────────
  PCHECK(pipe(g_signal_pipe) == 0) << "Failed to create signal pipe";

  struct sigaction sa = {};
  sa.sa_handler = SignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  PCHECK(sigaction(SIGINT, &sa, nullptr) == 0);
  PCHECK(sigaction(SIGTERM, &sa, nullptr) == 0);
  LOG(INFO) << "[UIApp] Signal handlers installed (SIGINT, SIGTERM).";

  base::RunLoop loop;

  auto signal_watcher = base::FileDescriptorWatcher::WatchReadable(
      g_signal_pipe[0],
      base::BindRepeating(
          [](int fd, base::RepeatingClosure quit) {
            char buf[1];
            HANDLE_EINTR(read(fd, buf, sizeof(buf)));
            LOG(INFO) << "[UIApp] Shutdown signal — quitting.";
            quit.Run();
          },
          g_signal_pipe[0], loop.QuitClosure()));

  const std::string agent_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII("agent-id");
  if (agent_id.empty()) {
    LOG(ERROR) << "[UIApp] --agent-id=<id> is required.";
    return 1;
  }
  const std::string agent_server_name =
      std::string(kAgentServerNamePrefix) + agent_id;

  UIAppApp app(agent_server_name);
  app.Start();

  LOG(INFO) << "[UIApp] Entering main run loop.";
  loop.Run();

  LOG(INFO) << "[UIApp] Exiting cleanly.";
  signal_watcher.reset();
  close(g_signal_pipe[0]);
  close(g_signal_pipe[1]);
  base::ThreadPoolInstance::Get()->Shutdown();
  return 0;
}