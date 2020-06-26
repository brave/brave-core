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

#include <utility>

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

}  // namespace

namespace ipfs {

IpfsServiceImpl::IpfsServiceImpl(
    mojo::PendingReceiver<mojom::IpfsService> receiver)
  : receiver_(this, std::move(receiver)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&IpfsServiceImpl::Cleanup, base::Unretained(this)));
#if defined(OS_POSIX)
  SetupPipeHack();
#endif
}

IpfsServiceImpl::~IpfsServiceImpl() {
  Cleanup();
}

void IpfsServiceImpl::Cleanup() {
  if (in_shutdown_) return;
  in_shutdown_ = true;

  if (ipfs_process_.IsValid()) {
    ipfs_process_.Terminate(0, true);
#if defined(OS_POSIX)
    TearDownPipeHack();
#endif
#if defined(OS_MACOSX)
    base::PostTask(
        FROM_HERE,
        {base::ThreadPool(),
         base::MayBlock(),
         base::TaskPriority::BEST_EFFORT},
        base::BindOnce(
          &base::EnsureProcessTerminated, Passed(&ipfs_process_)));
#else
    base::EnsureProcessTerminated(std::move(ipfs_process_));
#endif
  }
}

void IpfsServiceImpl::Launch(
    mojom::IpfsConfigPtr config, LaunchCallback callback) {
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
    base::CommandLine args(config->binary_path);
    args.AppendArg("init");
    base::Process init_process = base::LaunchProcess(args, options);
    if (!init_process.IsValid()) {
      std::move(callback).Run(false, init_process.Pid());
      return;
    }

    int exit_code = 0;
    if (!init_process.WaitForExit(&exit_code)) {
      VLOG(0) << "Failed to wait init process";
      init_process.Close();
      std::move(callback).Run(false, init_process.Pid());
      return;
    }
    if (exit_code) {
      VLOG(0) << "Failed at running init";
      std::move(callback).Run(false, init_process.Pid());
      return;
    }
  }

  // Launch IPFS daemon.
  base::CommandLine args(config->binary_path);
  args.AppendArg("daemon");
  args.AppendArg("--migrate=true");
  ipfs_process_ = base::LaunchProcess(args, options);
  bool result = ipfs_process_.IsValid();

  if (callback)
    std::move(callback).Run(result, ipfs_process_.Pid());

  if (!child_monitor_thread_.get()) {
    child_monitor_thread_.reset(new base::Thread("child_monitor_thread"));
    if (!child_monitor_thread_->Start()) {
      NOTREACHED();
    }

    child_monitor_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&IpfsServiceImpl::MonitorChild, base::Unretained(this)));
  }
}

void IpfsServiceImpl::MonitorChild() {
#if defined(OS_POSIX)
  char buf[PIPE_BUF];

  while (1) {
      if (read(pipehack[0], buf, sizeof(buf)) > 0) {
        pid_t pid;
        int status;

        if ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
          if (WIFSIGNALED(status)) {
            LOG(ERROR) << "ipfs got terminated by signal " << WTERMSIG(status);
          } else if (WCOREDUMP(status)) {
            LOG(ERROR) << "ipfs coredumped";
          } else if (WIFEXITED(status)) {
            LOG(ERROR) << "ipfs exit (" << WEXITSTATUS(status) << ")";
          }
          ipfs_process_.Close();
          if (receiver_.is_bound() && crash_handler_callback_) {
            base::ThreadTaskRunnerHandle::Get()->PostTask(
              FROM_HERE, base::BindOnce(std::move(crash_handler_callback_),
                                        pid));
          }
        }
      } else {
        // pipes closed
        break;
      }
  }
#elif defined(OS_WIN)
  WaitForSingleObject(ipfs_process_.Handle(), INFINITE);
  if (receiver_.is_bound() && crash_handler_callback_)
    base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(crash_handler_callback_),
                                base::GetProcId(ipfs_process_.Handle())));
#else
#error unsupported platforms
#endif
}

void IpfsServiceImpl::Shutdown() {
  Cleanup();
}

void IpfsServiceImpl::SetCrashHandler(SetCrashHandlerCallback callback) {
  crash_handler_callback_ = std::move(callback);
}

}  // namespace ipfs
