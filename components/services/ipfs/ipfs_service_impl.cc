/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ipfs/ipfs_service_impl.h"

#if defined(OS_LINUX)
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <initializer_list>
#include <string>
#include <utility>

#include "base/bind_post_task.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"

namespace {

#if defined(OS_POSIX)
int pipehack[2];

static void SIGCHLDHandler(int signo) {
  int error = errno;
  char ch = 0;
  (void)write(pipehack[1], &ch, 1);
  errno = error;
}

static void SetupPipeHack() {
  if (pipe(pipehack) == -1)
    VLOG(0) << "pipehack errno:" << errno;

  int flags;
  for (size_t i = 0; i < 2; ++i) {
    if ((flags = fcntl(pipehack[i], F_GETFL)) == -1)
      VLOG(0) << "get flags errno:" << errno;
    // Nonblock write end on SIGCHLD handler which will notify monitor thread
    // by sending one byte to pipe whose read end is blocked and wait for
    // SIGCHLD to arrives to avoid busy reading
    if (i == 1)
      flags |= O_NONBLOCK;
    if (fcntl(pipehack[i], F_SETFL, flags) == -1)
      VLOG(0) << "set flags errno:" << errno;
    if ((flags = fcntl(pipehack[i], F_GETFD)) == -1)
      VLOG(0) << "get fd flags errno:" << errno;
    flags |= FD_CLOEXEC;
    if (fcntl(pipehack[i], F_SETFD, flags) == -1)
      VLOG(0) << "set fd flags errno:" << errno;
  }

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGCHLDHandler;
  sigaction(SIGCHLD, &action, NULL);
}

static void TearDownPipeHack() {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &action, NULL);
  for (size_t i = 0; i < 2; ++i)
    close(pipehack[i]);
}
#endif

bool LaunchProcessAndExit(const base::FilePath& path,
                          std::initializer_list<std::string> args,
                          const base::LaunchOptions& options) {
  bool shutdown = false;
  base::CommandLine cmdline(path);
  for (auto arg : args) {
    if (arg == "shutdown")
      shutdown = true;
    cmdline.AppendArg(arg);
  }
  base::Process process = base::LaunchProcess(cmdline, options);
  if (!process.IsValid()) {
    return false;
  }

  int exit_code = 0;
  if (!process.WaitForExit(&exit_code)) {
    VLOG(0) << "Failed to wait the process, cmd: "
            << cmdline.GetCommandLineString();
    process.Close();
    return false;
  }

  // `ipfs shutdown` could return error if daemon is not running.
  if (exit_code && !shutdown) {
    VLOG(0) << "Failed at running cmd: " << cmdline.GetCommandLineString();
    return false;
  }

  return true;
}

void MonitorChild(base::ProcessHandle p_handle,
                  base::OnceCallback<void(base::ProcessId)> callback) {
#if defined(OS_POSIX)
  char buf[PIPE_BUF];

  while (1) {
    if (read(pipehack[0], buf, sizeof(buf)) > 0) {
      pid_t pid;
      int status;

      if ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
        if (WIFSIGNALED(status)) {
          VLOG(0) << "ipfs got terminated by signal " << WTERMSIG(status);
        } else if (WCOREDUMP(status)) {
          VLOG(0) << "ipfs coredumped";
        } else if (WIFEXITED(status)) {
          VLOG(0) << "ipfs exit (" << WEXITSTATUS(status) << ")";
        }
        if (callback) {
          std::move(callback).Run(pid);
        }
      }
    } else {
      // pipes closed
      break;
    }
  }
#elif defined(OS_WIN)
  WaitForSingleObject(p_handle, INFINITE);
  if (callback) {
    std::move(callback).Run(base::GetProcId(p_handle));
  }
#else
#error unsupported platforms
#endif
}

}  // namespace

namespace ipfs {

IpfsServiceImpl::IpfsServiceImpl(
    mojo::PendingReceiver<mojom::IpfsService> receiver)
    : receiver_(this, std::move(receiver)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receiver_.set_disconnect_handler(base::BindOnce(
      &IpfsServiceImpl::Cleanup, weak_ptr_factory_.GetWeakPtr()));
}

IpfsServiceImpl::~IpfsServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Cleanup();
}

void IpfsServiceImpl::Cleanup() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (in_shutdown_)
    return;
  in_shutdown_ = true;

  int64_t pid = -1;
  if (ipfs_process_.IsValid()) {
    pid = ipfs_process_.Pid();
    ipfs_process_.Terminate(0, true);
#if defined(OS_POSIX)
    TearDownPipeHack();
#endif
    base::EnsureProcessTerminated(std::move(ipfs_process_));
  }
  if (crash_handler_callback_)
    std::move(crash_handler_callback_).Run(pid);
}

void IpfsServiceImpl::Launch(mojom::IpfsConfigPtr config,
                             LaunchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (in_shutdown_) {
    if (callback)
      std::move(callback).Run(false, -1);
    return;
  }

  base::FilePath data_path = config->data_root_path;
  if (!base::DirectoryExists(data_path)) {
    DCHECK(base::CreateDirectory(data_path));
  }

  base::LaunchOptions options;
#if defined(OS_WIN)
  options.environment[L"IPFS_PATH"] = data_path.value();
#else
  options.environment["IPFS_PATH"] = data_path.value();
#endif
#if defined(OS_LINUX)
  options.kill_on_parent_death = true;
#endif
#if defined(OS_WIN)
  options.start_hidden = true;
#endif

  // Check if IPFS configs are ready, if not, run ipfs init to initialize them.
  base::FilePath config_path = config->config_path;
  if (!base::PathExists(config_path)) {
    // run ipfs init to gen config
    if (!LaunchProcessAndExit(config->binary_path, {"init"}, options)) {
      std::move(callback).Run(false, -1);
      return;
    }
  }

  std::initializer_list<std::initializer_list<std::string>> config_args = {
      {"shutdown"},  // Cleanup left-over daemon process.
      {"config", "Addresses.API", "/ip4/127.0.0.1/tcp/" + config->api_port},
      {"config", "Addresses.Gateway",
       "/ip4/127.0.0.1/tcp/" + config->gateway_port},
      {"config", "--json", "Addresses.Swarm",
       "[\"/ip4/0.0.0.0/tcp/" + config->swarm_port + "\", \"/ip6/::/tcp/" +
           config->swarm_port + "\"]"},
      {"config", "Datastore.GCPeriod", "1h"},
      {"config", "--json", "Swarm.ConnMgr.LowWater", "50"},
      {"config", "--json", "Swarm.ConnMgr.HighWater", "300"}};

  for (auto args : config_args) {
    if (!LaunchProcessAndExit(config->binary_path, args, options)) {
      std::move(callback).Run(false, -1);
      return;
    }
  }

  // Configure storage
  if (!LaunchProcessAndExit(
          config->binary_path,
          {"config", "Datastore.StorageMax", config->storage_max}, options)) {
    std::move(callback).Run(false, -1);
    return;
  }

#if defined(OS_POSIX)
  // Setup pipe hack here to monitor the daemon process and exclude other child
  // processes created above.
  SetupPipeHack();
#endif

  // Launch IPFS daemon.
  base::CommandLine args(config->binary_path);
  args.AppendArg("daemon");
  args.AppendArg("--migrate=true");
  args.AppendArg("--enable-gc");
  args.AppendArg("--routing=dhtclient");
  ipfs_process_ = base::LaunchProcess(args, options);
  bool result = ipfs_process_.IsValid();

  if (callback)
    std::move(callback).Run(result, ipfs_process_.Pid());

  // No need to proceed if we fail to launch the daemon, Shutdown will be
  // called after IPFS service in browser process received failed result.
  if (!result) {
    return;
  }

  if (!child_monitor_thread_.get()) {
    child_monitor_thread_.reset(new base::Thread("child_monitor_thread"));
    if (!child_monitor_thread_->Start()) {
      NOTREACHED();
    }

    child_monitor_thread_->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&MonitorChild, ipfs_process_.Handle(),
                       base::BindPostTask(
                           base::SequencedTaskRunnerHandle::Get(),
                           base::BindOnce(&IpfsServiceImpl::OnChildCrash,
                                          weak_ptr_factory_.GetWeakPtr()))));
  }
}

void IpfsServiceImpl::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Cleanup();
}

void IpfsServiceImpl::SetCrashHandler(SetCrashHandlerCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  crash_handler_callback_ = std::move(callback);
}

void IpfsServiceImpl::OnChildCrash(base::ProcessId pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ipfs_process_.Close();
  if (receiver_.is_bound() && crash_handler_callback_ && !in_shutdown_)
    std::move(crash_handler_callback_).Run(pid);
}

}  // namespace ipfs
