#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/path_service.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/browser/brave_vpn/poc_wireguard/boringtun_loader.h"
#include "brave/browser/brave_vpn/poc_wireguard/platform_tools.h"
#include "brave/browser/brave_vpn/poc_wireguard/tun_interface.h"
#include "brave/browser/brave_vpn/poc_wireguard/udp_transport.h"
#include "brave/browser/brave_vpn/poc_wireguard/wireguard_config_parser.h"
#include "brave/browser/brave_vpn/poc_wireguard/wireguard_tunnel.h"

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_STDERR;
  logging::InitLogging(settings);
  logging::SetMinLogLevel(logging::LOGGING_INFO);

  // Must be called before any threads are spawned so the signal mask is
  // inherited correctly (POSIX). No-op on Windows.
  wireguard::InitShutdownSignal();

  LOG(INFO) << "WireGuard POC - pid=" << base::GetCurrentProcId();

  if (!wireguard::IsElevated()) {
    LOG(WARNING)
        << "Not running as administrator - TUN creation will likely fail.";
  }

  const base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  const base::CommandLine::StringVector& args = cmd->GetArgs();
  if (args.empty()) {
    LOG(ERROR) << "Usage: poc_wireguard <config.conf>";
    return 1;
  }
  const std::string config_path = base::FilePath(args[0]).AsUTF8Unsafe();

  wireguard::WireGuardConfig config;
  if (!wireguard::ParseConfigFile(config_path, config)) {
    LOG(ERROR) << "Failed to parse config: " << config_path;
    return 1;
  }
  wireguard::LogConfig(config);

  if (!wireguard::ResolveEndpoint(&config)) {
    LOG(ERROR) << "Failed to resolve endpoint hostname";
    return 1;
  }

  // The main thread's IO executor is available for the privileged helper's
  // other work (XPC on macOS, named pipe on Windows, etc.).
  // The tunnel runs on its own dedicated thread.
  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::IO);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("wireguard-poc");

  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  const std::string lib_path =
      exe_dir.AppendASCII(wireguard::BoringtunLibraryName()).AsUTF8Unsafe();

  auto loader = wireguard::CreateBoringtunLoader();
  if (!loader->Load(lib_path)) {
    LOG(ERROR) << "Failed to load boringtun from: " << lib_path;
    return 1;
  }

  auto tun = wireguard::CreateTunInterface();
  auto udp = wireguard::CreateUdpTransport();
  wireguard::WireGuardTunnel tunnel(loader.get(), std::move(tun),
                                    std::move(udp));

  if (!tunnel.Init(config)) {
    LOG(ERROR) << "Tunnel initialization failed";
    return 1;
  }
  if (!tunnel.Start()) {
    LOG(ERROR) << "Tunnel failed to start";
    return 1;
  }

  LOG(INFO) << "Tunnel up - main thread free (Ctrl-C to stop)";

  const int sig = wireguard::WaitForShutdownSignal();
  LOG(INFO) << "Shutdown signal " << sig << " received - stopping tunnel";

  tunnel.Stop();
  LOG(INFO) << "Exiting cleanly";
  return 0;
}
