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
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "brave/browser/brave_vpn/poc-mojo/ipc/poc.mojom.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "components/named_mojo_ipc_server/endpoint_options.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server.h"
#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"

namespace {

#if BUILDFLAG(IS_WIN)
const wchar_t kServerName[] = L"poc_helper_ipc";
#else
constexpr char kServerName[] = "poc_helper_ipc";
#endif

// Identifies which pipe slot inside the Mojo invitation envelope carries the
// HelperService interface.  Both helper and agent must agree on this value at
// compile time; it is a label, not a secret.
// Pre-shared authentication token.
// ⚠ PRODUCTION: replace with a runtime-generated secret passed over a secure
//   side-channel, and use a constant-time compare (e.g. memcmp_s).
constexpr char kAuthToken[] = "poc-shared-secret-do-not-use-in-production";

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
    LOG(INFO) << "[Helper] Shutdown signal - quitting.";
    g_main_task_runner->PostTask(FROM_HERE, *g_quit_closure);
    return TRUE;
  }
  return FALSE;
}
#endif  // BUILDFLAG(IS_WIN)

// Forward declaration so HelperServiceImpl can reference HelperApp.
class HelperApp;

// ============================================================================
// HelperServiceImpl — one shared instance; per-client state lives in
// HelperApp::clients_, keyed by mojo::ReceiverId.
//
// ReceiverId is not available at connection time (it is assigned inside
// NamedMojoIpcServer after impl_provider returns).  The earliest safe place
// to read it is inside the first interface method call, which is always
// Authenticate().  Unauthenticated clients therefore have no entry in
// clients_; a missing entry is the sentinel for "not yet authenticated."
// ============================================================================

class HelperServiceImpl : public poc::mojom::HelperService {
 public:
  explicit HelperServiceImpl(HelperApp* app) : app_(app) {}

 private:
  void Authenticate(const std::string& token,
                    mojo::PendingRemote<poc::mojom::AgentService> agent_remote,
                    AuthenticateCallback callback) override;

  void TestHelper(TestHelperCallback callback) override;

  const raw_ptr<HelperApp> app_;
};

// ============================================================================
// HelperApp
// ============================================================================

class HelperApp {
 public:
  // Per-client state, created on successful Authenticate() and erased on
  // disconnect.  `authenticated` is always true for entries in clients_ —
  // the map itself is the authentication sentinel.
  struct ClientState {
    mojo::Remote<poc::mojom::AgentService> agent_service;
  };

  HelperApp()
      : impl_(this),
        server_(
            named_mojo_ipc_server::EndpointOptions(
                kServerName,
                named_mojo_ipc_server::EndpointOptions::kUseIsolatedConnection),
            base::BindRepeating(&HelperApp::OnClientConnecting,
                                base::Unretained(this))) {
    server_.set_disconnect_handler(base::BindRepeating(
        &HelperApp::OnClientDisconnected, base::Unretained(this)));
  }

  void Start() {
    LOG(INFO) << "[Helper] Starting (multi-client).";
    server_.StartServer();
  }

  // Accessors for HelperServiceImpl, which is defined before HelperApp but
  // needs to call into it.
  auto& server() { return server_; }
  auto& clients() { return clients_; }

  // Called from HelperServiceImpl::Authenticate() when the AgentService remote
  // that was passed during auth disconnects.  server_.Close() removes the
  // receiver without firing the global disconnect handler (ReceiverSet::Remove
  // does not trigger it), so we erase clients_ here directly.
  void OnAgentServiceDisconnected(mojo::ReceiverId id) {
    LOG(INFO) << "[Helper] AgentService for client " << id << " disconnected.";
    server_.Close(id);
    clients_.erase(id);
    LOG(INFO) << "[Helper] Client " << id
              << " removed (active: " << clients_.size() << ").";
  }

  // Called from HelperServiceImpl::TestHelper() to call back the specific
  // agent that issued the TestHelper request.
  void CallTestAgent(mojo::ReceiverId id) {
    auto it = clients_.find(id);
    if (it == clients_.end() || !it->second.agent_service.is_connected()) {
      LOG(WARNING) << "[Helper] CallTestAgent: client " << id << " gone.";
      return;
    }
    LOG(INFO) << "[Helper] Calling TestAgent() on client " << id << ".";
    it->second.agent_service->TestAgent(base::BindOnce(
        &HelperApp::OnTestAgentResponse, weak_factory_.GetWeakPtr(), id));
  }

  // Broadcasts TestAgent() to every authenticated, connected agent.
  void BroadcastTestAgent() {
    for (auto& [id, state] : clients_) {
      if (!state.agent_service.is_connected()) {
        continue;
      }
      LOG(INFO) << "[Helper] Broadcasting TestAgent() to client " << id << ".";
      state.agent_service->TestAgent(base::BindOnce(
          &HelperApp::OnTestAgentResponse, weak_factory_.GetWeakPtr(), id));
    }
  }

 private:
  // impl_provider callback — called by NamedMojoIpcServer for every new
  // connection before a ReceiverId is assigned.  Returning nullptr would
  // reject the connection; we always accept here and let Authenticate() enforce
  // the token check.
  poc::mojom::HelperService* OnClientConnecting(
      const named_mojo_ipc_server::ConnectionInfo& info) {
    LOG(INFO) << "[Helper] New client connecting (pid " << info.pid << ").";
    return &impl_;
  }

  // Fired by NamedMojoIpcServer on natural pipe close (client process dies or
  // disconnects cleanly).  Not fired by server_.Close() — see
  // OnAgentServiceDisconnected for that path.
  void OnClientDisconnected() {
    const auto id = server_.current_receiver();
    // Entry may already be absent if the agent service disconnected first and
    // OnAgentServiceDisconnected already cleaned up.
    const bool had_entry = clients_.erase(id) > 0;
    LOG(INFO) << "[Helper] Client " << id
              << (had_entry ? " removed"
                            : " disconnected (never authenticated)")
              << " (active: " << clients_.size() << ").";
  }

  void OnTestAgentResponse(mojo::ReceiverId id, const std::string& response) {
    LOG(INFO) << "[Helper] TestAgent() from client " << id << ": \"" << response
              << "\".";
  }

  HelperServiceImpl impl_;
  named_mojo_ipc_server::NamedMojoIpcServer<poc::mojom::HelperService> server_;
  base::flat_map<mojo::ReceiverId, ClientState> clients_;
  base::WeakPtrFactory<HelperApp> weak_factory_{this};
};

// ============================================================================
// HelperServiceImpl method bodies (defined after HelperApp is complete so the
// full class definition is visible)
// ============================================================================

void HelperServiceImpl::Authenticate(
    const std::string& token,
    mojo::PendingRemote<poc::mojom::AgentService> agent_remote,
    AuthenticateCallback callback) {
  const auto id = app_->server().current_receiver();
  auto& clients = app_->clients();

  if (clients.contains(id)) {
    LOG(WARNING) << "[Helper] Client " << id
                 << ": duplicate Authenticate() — closing.";
    std::move(callback).Run(false);
    app_->server().Close(id);
    return;
  }

  // ⚠ PRODUCTION: constant-time token comparison.
  if (token != kAuthToken) {
    LOG(WARNING) << "[Helper] Client " << id << ": bad token — closing.";
    std::move(callback).Run(false);
    app_->server().Close(id);
    return;
  }

  auto& state = clients[id];
  state.agent_service.Bind(std::move(agent_remote));
  // Capture id by value — ReceiverId is a plain integer, safe across any
  // reconnect cycle, and base::Unretained(app_) is safe because HelperApp
  // outlives all connections.
  state.agent_service.set_disconnect_handler(base::BindOnce(
      &HelperApp::OnAgentServiceDisconnected, base::Unretained(app_), id));

  LOG(INFO) << "[Helper] Client " << id
            << " authenticated (active: " << clients.size() << ").";
  std::move(callback).Run(true);
}

void HelperServiceImpl::TestHelper(TestHelperCallback callback) {
  const auto id = app_->server().current_receiver();

  if (!app_->clients().contains(id)) {
    LOG(WARNING) << "[Helper] Client " << id
                 << ": TestHelper() before auth — closing.";
    app_->server().Close(id);
    return;
  }

  LOG(INFO) << "[Helper] TestHelper() from client " << id << ".";
  std::move(callback).Run("Hello from helper");

  // Schedule the reverse call 1 s later.  base::Unretained is safe: HelperApp
  // outlives all task runners.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&HelperApp::CallTestAgent, base::Unretained(app_), id),
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

  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("poc_helper");

  base::SingleThreadTaskExecutor main_executor(base::MessagePumpType::IO);
#if BUILDFLAG(IS_POSIX)
  base::FileDescriptorWatcher fd_watcher(main_executor.task_runner());
#endif

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
  LOG(INFO) << "[Helper] Signal handlers installed (SIGINT, SIGTERM).";

  auto signal_watcher = base::FileDescriptorWatcher::WatchReadable(
      g_signal_pipe[0], base::BindRepeating(
                            [](int fd, base::RepeatingClosure quit) {
                              char buf[1];
                              HANDLE_EINTR(read(fd, buf, sizeof(buf)));
                              LOG(INFO)
                                  << "[Helper] Shutdown signal — quitting.";
                              quit.Run();
                            },
                            g_signal_pipe[0], loop.QuitClosure()));
#elif BUILDFLAG(IS_WIN)
  auto quit_closure = loop.QuitClosure();
  g_main_task_runner = base::SequencedTaskRunner::GetCurrentDefault().get();
  g_quit_closure = &quit_closure;
  PCHECK(SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
      << "Failed to install console ctrl handler";
  LOG(INFO) << "[Helper] Console ctrl handler installed (Ctrl+C, Ctrl+Break).";
#endif  // signal handling

  HelperApp app;
  app.Start();

  LOG(INFO) << "[Helper] Entering main run loop.";
  loop.Run();

  LOG(INFO) << "[Helper] Exiting cleanly.";
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
