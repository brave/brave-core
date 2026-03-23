// Copyright 2024 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#if BUILDFLAG(IS_POSIX)
#include <signal.h>

#include "base/files/file_descriptor_watcher_posix.h"
#include "base/posix/eintr_wrapper.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "brave/browser/brave_vpn/poc-mojo/ipc/poc.mojom.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "components/named_mojo_ipc_server/endpoint_options.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server_client_util.h"
#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"

namespace {

#if BUILDFLAG(IS_WIN)
const wchar_t kHelperServerName[] = L"poc_helper_ipc";
const wchar_t kAgentServerNamePrefix[] = L"poc_agent_ipc_";
#else
constexpr char kHelperServerName[] = "poc_helper_ipc";
constexpr char kAgentServerNamePrefix[] = "poc_agent_ipc_";
#endif

constexpr char kHelperAuthToken[] =
    "poc-shared-secret-do-not-use-in-production";
// ⚠ PRODUCTION: separate token for uiapp auth, runtime-generated.
constexpr char kUIAppAuthToken[] = "poc-uiapp-secret-do-not-use-in-production";

constexpr base::TimeDelta kReconnectDelay = base::Seconds(1);
constexpr base::TimeDelta kTestCycleInterval = base::Seconds(3);

// ── Platform signal handling ─────────────────────────────────────────────────
// POSIX: self-pipe trick — signal handler writes a byte; FileDescriptorWatcher
//   wakes the IO RunLoop on the main thread to call QuitClosure safely.
//
// Windows: SetConsoleCtrlHandler fires on a separate OS thread.  We capture
//   the main-thread task runner and post QuitClosure back to it.
#if BUILDFLAG(IS_POSIX)
int g_signal_pipe[2] = {-1, -1};
void SignalHandler(int /*sig*/) {
  char byte = 1;
  write(g_signal_pipe[1], &byte, 1);  // NOLINT(cert-err33-c)
}
#elif BUILDFLAG(IS_WIN)
// Accessed from the OS console-handler thread — must be set before
// SetConsoleCtrlHandler is called and never mutated afterward.
base::SequencedTaskRunner* g_main_task_runner = nullptr;
base::RepeatingClosure* g_quit_closure = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD type) {
  if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT ||
      type == CTRL_CLOSE_EVENT) {
    LOG(INFO) << "[Agent] Shutdown signal - quitting.";
    g_main_task_runner->PostTask(FROM_HERE, *g_quit_closure);
    return TRUE;
  }
  return FALSE;
}
#endif  // BUILDFLAG(IS_WIN)

// Forward declaration.
class AgentApp;

// ============================================================================
// AgentServiceImpl — exposed to helper.
// ============================================================================

class AgentServiceImpl : public poc::mojom::AgentService {
 public:
  void TestAgent(TestAgentCallback callback) override {
    LOG(INFO) << "[Agent] TestAgent() received. Sending response.";
    std::move(callback).Run("Hello from agent");
  }
};

// ============================================================================
// AgentUserServiceImpl — shared impl exposed to uiapps via
// NamedMojoIpcServer.  Per-uiapp state lives in AgentApp::uiapp_clients_.
// ============================================================================

class AgentUserServiceImpl : public poc::mojom::AgentUserService {
 public:
  explicit AgentUserServiceImpl(AgentApp* app) : app_(app) {}

 private:
  void Authenticate(const std::string& token,
                    mojo::PendingRemote<poc::mojom::UIAppService> uiapp_remote,
                    AuthenticateCallback callback) override;

  void TestAgentUser(TestAgentUserCallback callback) override;

  const raw_ptr<AgentApp> app_;
};

// ============================================================================
// AgentApp
// ============================================================================

class AgentApp {
 public:
  // Per-uiapp state, created on successful Authenticate().
  struct UIAppClientState {
    mojo::Remote<poc::mojom::UIAppService> uiapp_service;
  };

  explicit AgentApp(const mojo::NamedPlatformChannel::ServerName& server_name)
      : agent_receiver_(&agent_impl_),
        uiapp_server_impl_(this),
        uiapp_server_(
            named_mojo_ipc_server::EndpointOptions(
                server_name,
                named_mojo_ipc_server::EndpointOptions::kUseIsolatedConnection),
            base::BindRepeating(&AgentApp::OnUIAppConnecting,
                                base::Unretained(this))) {
    uiapp_server_.set_disconnect_handler(base::BindRepeating(
        &AgentApp::OnUIAppDisconnected, base::Unretained(this)));
  }

  void Start() {
    LOG(INFO) << "[Agent] Starting.";
    uiapp_server_.StartServer();
    TryConnect();
  }

  auto& uiapp_server() { return uiapp_server_; }
  auto& uiapp_clients() { return uiapp_clients_; }

  void OnUIAppServiceDisconnected(mojo::ReceiverId id) {
    LOG(INFO) << "[Agent] UIAppService remote for client " << id
              << " disconnected.";
    uiapp_server_.Close(id);
    uiapp_clients_.erase(id);
    LOG(INFO) << "[Agent] UIApp client " << id
              << " removed (active: " << uiapp_clients_.size() << ").";
  }

  // Calls TestUIApp() on a specific uiapp client.
  void CallTestUIApp(mojo::ReceiverId id) {
    auto it = uiapp_clients_.find(id);
    if (it == uiapp_clients_.end() ||
        !it->second.uiapp_service.is_connected()) {
      LOG(WARNING) << "[Agent] CallTestUIApp: client " << id << " gone.";
      return;
    }
    LOG(INFO) << "[Agent] Calling TestUIApp() on uiapp client " << id << ".";
    it->second.uiapp_service->TestUIApp(base::BindOnce(
        &AgentApp::OnTestUIAppResponse, weak_factory_.GetWeakPtr(), id));
  }

  // Broadcasts TestUIApp() to all authenticated uiapp clients.
  void BroadcastTestUIApp() {
    for (auto& [id, state] : uiapp_clients_) {
      if (!state.uiapp_service.is_connected()) {
        continue;
      }
      LOG(INFO) << "[Agent] Broadcasting TestUIApp() to client " << id << ".";
      state.uiapp_service->TestUIApp(base::BindOnce(
          &AgentApp::OnTestUIAppResponse, weak_factory_.GetWeakPtr(), id));
    }
  }

 private:
  // ── NamedMojoIpcServer callbacks ──────────────────────────────────────────

  poc::mojom::AgentUserService* OnUIAppConnecting(
      const named_mojo_ipc_server::ConnectionInfo& info) {
    LOG(INFO) << "[Agent] New uiapp connecting (pid " << info.pid << ").";
    return &uiapp_server_impl_;
  }

  void OnUIAppDisconnected() {
    const auto id = uiapp_server_.current_receiver();
    const bool had_entry = uiapp_clients_.erase(id) > 0;
    LOG(INFO) << "[Agent] UIApp client " << id
              << (had_entry ? " removed"
                            : " disconnected (never authenticated)")
              << " (active: " << uiapp_clients_.size() << ").";
  }

  void OnTestUIAppResponse(mojo::ReceiverId id, const std::string& response) {
    LOG(INFO) << "[Agent] TestUIApp() from client " << id << ": \"" << response
              << "\".";
  }

  // ── Helper client ─────────────────────────────────────────────────────────

  void TryConnect() {
    Reset();

    LOG(INFO) << "[Agent] Connecting to \"" << kHelperServerName << "\"...";

    mojo::PlatformChannelEndpoint endpoint =
        named_mojo_ipc_server::ConnectToServer(kHelperServerName);

    if (!endpoint.is_valid()) {
      LOG(INFO) << "[Agent] Helper not reachable. Retrying in "
                << kReconnectDelay << ".";
      ScheduleReconnect();
      return;
    }

    mojo::ScopedMessagePipeHandle pipe =
        mojo::IncomingInvitation::AcceptIsolated(std::move(endpoint));

    if (!pipe.is_valid()) {
      LOG(ERROR) << "[Agent] AcceptIsolated returned invalid pipe.";
      ScheduleReconnect();
      return;
    }

    helper_service_.Bind(
        mojo::PendingRemote<poc::mojom::HelperService>(std::move(pipe), 0));
    helper_service_.set_disconnect_handler(base::BindOnce(
        &AgentApp::OnHelperDisconnected, weak_factory_.GetWeakPtr()));

    mojo::PendingRemote<poc::mojom::AgentService> agent_remote;
    agent_receiver_.Bind(agent_remote.InitWithNewPipeAndPassReceiver());
    agent_receiver_.set_disconnect_handler(base::BindOnce(
        &AgentApp::OnAgentReceiverDisconnected, weak_factory_.GetWeakPtr()));

    LOG(INFO) << "[Agent] Sending Authenticate().";
    helper_service_->Authenticate(
        kHelperAuthToken, std::move(agent_remote),
        base::BindOnce(&AgentApp::OnAuthenticated, weak_factory_.GetWeakPtr()));
  }

  void OnAuthenticated(bool accepted) {
    if (!accepted) {
      LOG(ERROR) << "[Agent] Authentication rejected by helper. Retrying in "
                 << kReconnectDelay << ".";
      ScheduleReconnect();
      return;
    }
    LOG(INFO) << "[Agent] Authenticated with helper. Starting test cycle.";
    CallTestHelper();
  }

  void CallTestHelper() {
    if (!helper_service_.is_bound() || !helper_service_.is_connected()) {
      LOG(WARNING) << "[Agent] Helper gone; stopping cycle.";
      return;
    }
    LOG(INFO) << "[Agent] Calling TestHelper().";
    helper_service_->TestHelper(base::BindOnce(&AgentApp::OnTestHelperResponse,
                                               weak_factory_.GetWeakPtr()));
  }

  void OnTestHelperResponse(const std::string& response) {
    LOG(INFO) << "[Agent] TestHelper() returned: \"" << response << "\".";
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&AgentApp::CallTestHelper, weak_factory_.GetWeakPtr()),
        kTestCycleInterval);
  }

  void OnHelperDisconnected() {
    LOG(INFO) << "[Agent] Helper disconnected. Reconnecting in "
              << kReconnectDelay << ".";
    ScheduleReconnect();
  }

  void OnAgentReceiverDisconnected() {
    LOG(INFO) << "[Agent] AgentService receiver disconnected.";
  }

  void Reset() {
    helper_service_.reset();
    if (agent_receiver_.is_bound()) {
      agent_receiver_.reset();
    }
  }

  void ScheduleReconnect() {
    weak_factory_.InvalidateWeakPtrs();
    Reset();
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&AgentApp::TryConnect, weak_factory_.GetWeakPtr()),
        kReconnectDelay);
  }

  // ── Helper client state ───────────────────────────────────────────────────
  mojo::Remote<poc::mojom::HelperService> helper_service_;
  AgentServiceImpl agent_impl_;
  mojo::Receiver<poc::mojom::AgentService> agent_receiver_;

  // ── UIApp server state ────────────────────────────────────────────────────
  AgentUserServiceImpl uiapp_server_impl_;
  named_mojo_ipc_server::NamedMojoIpcServer<poc::mojom::AgentUserService>
      uiapp_server_;
  base::flat_map<mojo::ReceiverId, UIAppClientState> uiapp_clients_;

  base::WeakPtrFactory<AgentApp> weak_factory_{this};
};

// ============================================================================
// AgentUserServiceImpl method bodies
// ============================================================================

void AgentUserServiceImpl::Authenticate(
    const std::string& token,
    mojo::PendingRemote<poc::mojom::UIAppService> uiapp_remote,
    AuthenticateCallback callback) {
  const auto id = app_->uiapp_server().current_receiver();
  auto& clients = app_->uiapp_clients();

  if (clients.contains(id)) {
    LOG(WARNING) << "[Agent] UIApp client " << id
                 << ": duplicate Authenticate() — closing.";
    std::move(callback).Run(false);
    app_->uiapp_server().Close(id);
    return;
  }

  // ⚠ PRODUCTION: constant-time token comparison.
  if (token != kUIAppAuthToken) {
    LOG(WARNING) << "[Agent] UIApp client " << id << ": bad token — closing.";
    std::move(callback).Run(false);
    app_->uiapp_server().Close(id);
    return;
  }

  auto& state = clients[id];
  state.uiapp_service.Bind(std::move(uiapp_remote));
  state.uiapp_service.set_disconnect_handler(base::BindOnce(
      &AgentApp::OnUIAppServiceDisconnected, base::Unretained(app_), id));

  LOG(INFO) << "[Agent] UIApp client " << id
            << " authenticated (active: " << clients.size() << ").";
  std::move(callback).Run(true);
}

void AgentUserServiceImpl::TestAgentUser(TestAgentUserCallback callback) {
  const auto id = app_->uiapp_server().current_receiver();

  if (!app_->uiapp_clients().contains(id)) {
    LOG(WARNING) << "[Agent] UIApp client " << id
                 << ": TestAgentUser() before auth — closing.";
    app_->uiapp_server().Close(id);
    return;
  }

  LOG(INFO) << "[Agent] TestAgentUser() from uiapp client " << id << ".";
  std::move(callback).Run("Hello from agent");

  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AgentApp::CallTestUIApp, base::Unretained(app_), id),
      base::Seconds(1));
}

}  // namespace

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);

  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  logging::InitLogging(settings);
  logging::SetMinLogLevel(logging::LOGGING_INFO);

  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("poc_agent");

  base::SingleThreadTaskExecutor main_executor(base::MessagePumpType::IO);
#if BUILDFLAG(IS_POSIX)
  base::FileDescriptorWatcher fd_watcher(main_executor.task_runner());
#endif

  // Agent is a broker: it uses AcceptIsolated to connect to helper, and
  // hosts its own NamedMojoIpcServer for uiapps.
  mojo::core::Configuration config;
  config.is_broker_process = true;
  mojo::core::Init(config);

  base::Thread ipc_thread("MojoIPC");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessagePumpType::IO, 0));
  mojo::core::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  base::RunLoop loop;

  // ── Signal / console-interrupt handling ───────────────────────────────────
#if BUILDFLAG(IS_POSIX)
  PCHECK(pipe(g_signal_pipe) == 0) << "Failed to create signal pipe";

  struct sigaction sa = {};
  sa.sa_handler = SignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  PCHECK(sigaction(SIGINT, &sa, nullptr) == 0);
  PCHECK(sigaction(SIGTERM, &sa, nullptr) == 0);
  LOG(INFO) << "[Agent] Signal handlers installed (SIGINT, SIGTERM).";

  auto signal_watcher = base::FileDescriptorWatcher::WatchReadable(
      g_signal_pipe[0], base::BindRepeating(
                            [](int fd, base::RepeatingClosure quit) {
                              char buf[1];
                              HANDLE_EINTR(read(fd, buf, sizeof(buf)));
                              LOG(INFO)
                                  << "[Agent] Shutdown signal — quitting.";
                              quit.Run();
                            },
                            g_signal_pipe[0], loop.QuitClosure()));
#elif BUILDFLAG(IS_WIN)
  auto quit_closure = loop.QuitClosure();
  g_main_task_runner = base::SequencedTaskRunner::GetCurrentDefault().get();
  g_quit_closure = &quit_closure;
  PCHECK(SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
      << "Failed to install console ctrl handler";
  LOG(INFO) << "[Agent] Console ctrl handler installed (Ctrl+C, Ctrl+Break).";
#endif  // signal handling

  const auto token = base::UnguessableToken::Create();
  const auto server_name =
      mojo::NamedPlatformChannel::ServerName(kAgentServerNamePrefix)
#if BUILDFLAG(IS_WIN)
      + base::SysUTF8ToWide(token.ToString());
#else
      + token.ToString();
#endif

  AgentApp app(server_name);
  app.Start();

  LOG(INFO) << "[Agent] Agent ID: " << token.ToString()
            << "  (pass --agent-id=" << token.ToString() << " to uiapp)";

  LOG(INFO) << "[Agent] Entering main run loop.";
  loop.Run();

  LOG(INFO) << "[Agent] Exiting cleanly.";
#if BUILDFLAG(IS_POSIX)
  signal_watcher.reset();
  close(g_signal_pipe[0]);
  close(g_signal_pipe[1]);
#elif BUILDFLAG(IS_WIN)
  SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
#endif
  base::ThreadPoolInstance::Get()->Shutdown();
  return 0;
}
