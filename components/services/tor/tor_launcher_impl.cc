/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/tor/tor_launcher_impl.h"


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
    LOG(ERROR) << "pipehack errno:" << errno;

  int flags;
  for (size_t i = 0; i < 2; ++i) {
    if ((flags = fcntl(pipehack[i], F_GETFL)) == -1)
      LOG(ERROR) << "get flags errno:" << errno;
    // Nonblock write end on SIGCHLD handler which will notify monitor thread
    // by sending one byte to pipe whose read end is blocked and wait for
    // SIGCHLD to arrives to avoid busy reading
    if (i == 1)
      flags |= O_NONBLOCK;
    if (fcntl(pipehack[i], F_SETFL, flags) == -1)
      LOG(ERROR) << "set flags errno:" << errno;
    if ((flags = fcntl(pipehack[i], F_GETFD)) == -1)
      LOG(ERROR) << "get fd flags errno:" << errno;
    flags |= FD_CLOEXEC;
    if (fcntl(pipehack[i], F_SETFD, flags) == -1)
      LOG(ERROR) << "set fd flags errno:" << errno;
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

namespace tor {

TorLauncherImpl::TorLauncherImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {
#if defined(OS_POSIX)
  SetupPipeHack();
#endif
}

TorLauncherImpl::~TorLauncherImpl() {
  if (tor_process_.IsValid()) {
    tor_process_.Terminate(0, true);
#if defined(OS_POSIX)
    TearDownPipeHack();
#endif
#if defined(OS_MAC)
    base::PostTask(
        FROM_HERE,
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT},
        base::BindOnce(&base::EnsureProcessTerminated, Passed(&tor_process_)));
#else
    base::EnsureProcessTerminated(std::move(tor_process_));
#endif
  }
}

void TorLauncherImpl::Launch(const TorConfig& config,
                             LaunchCallback callback) {
  base::CommandLine args(config.binary_path());
  args.AppendArg("--ignore-missing-torrc");
  args.AppendArg("-f");
  args.AppendArg("/nonexistent");
  args.AppendArg("--defaults-torrc");
  args.AppendArg("/nonexistent");
  args.AppendArg("--SocksPort");
  args.AppendArg(config.proxy_host() + ":" + config.proxy_port());
  args.AppendArg("--TruncateLogFile");
  args.AppendArg("1");
  base::FilePath tor_data_path = config.tor_data_path();
  if (!tor_data_path.empty()) {
    if (!base::DirectoryExists(tor_data_path))
      base::CreateDirectory(tor_data_path);
    args.AppendArg("--DataDirectory");
    args.AppendArgPath(tor_data_path);
    args.AppendArg("--Log");
    base::CommandLine::StringType log_file;
    log_file += FILE_PATH_LITERAL("notice file ");
    args.AppendArgNative(log_file +
                         tor_data_path.AppendASCII("tor.log").value());
  }
  base::FilePath tor_watch_path = config.tor_watch_path();
  if (!tor_watch_path.empty()) {
    if (!base::DirectoryExists(tor_watch_path))
      base::CreateDirectory(tor_watch_path);
    args.AppendArg("--pidfile");
    args.AppendArgPath(tor_watch_path.AppendASCII("tor.pid"));
    args.AppendArg("--controlport");
    args.AppendArg("auto");
    args.AppendArg("--controlportwritetofile");
    args.AppendArgPath(tor_watch_path.AppendASCII("controlport"));
    args.AppendArg("--cookieauthentication");
    args.AppendArg("1");
    args.AppendArg("--cookieauthfile");
    args.AppendArgPath(tor_watch_path.AppendASCII("control_auth_cookie"));
  }

  base::LaunchOptions launchopts;
#if defined(OS_LINUX)
  launchopts.kill_on_parent_death = true;
#endif
#if defined(OS_WIN)
  launchopts.start_hidden = true;
#endif
  tor_process_ = base::LaunchProcess(args, launchopts);

  // TODO(darkdh): return success when tor connected to tor network
  bool result = tor_process_.IsValid();

  if (callback)
    std::move(callback).Run(result, tor_process_.Pid());

  if (!child_monitor_thread_.get()) {
    child_monitor_thread_.reset(new base::Thread("child_monitor_thread"));
    if (!child_monitor_thread_->Start()) {
      NOTREACHED();
    }

    child_monitor_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&TorLauncherImpl::MonitorChild, base::Unretained(this)));
  }
}

void TorLauncherImpl::SetCrashHandler(SetCrashHandlerCallback callback) {
  crash_handler_callback_ = std::move(callback);
}

void TorLauncherImpl::ReLaunch(const TorConfig& config,
                               ReLaunchCallback callback) {
  if (tor_process_.IsValid())
    tor_process_.Terminate(0, true);

  tor_process_.WaitForExit(nullptr);
  Launch(config, std::move(callback));
}

void TorLauncherImpl::MonitorChild() {
#if defined(OS_POSIX)
  char buf[PIPE_BUF];

  while (1) {
      if (read(pipehack[0], buf, sizeof(buf)) > 0) {
        pid_t pid;
        int status;

        if ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
          if (WIFSIGNALED(status)) {
            LOG(ERROR) << "tor got terminated by signal " << WTERMSIG(status);
          } else if (WCOREDUMP(status)) {
            LOG(ERROR) << "tor coredumped";
          } else if (WIFEXITED(status)) {
            LOG(ERROR) << "tor exit (" << WEXITSTATUS(status) << ")";
          }
          tor_process_.Close();
          if (connected_ && crash_handler_callback_) {
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
  WaitForSingleObject(tor_process_.Handle(), INFINITE);
  if (connected_ && crash_handler_callback_)
    base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(crash_handler_callback_),
                                base::GetProcId(tor_process_.Handle())));
#else
#error unsupported platforms
#endif
}

void TorLauncherImpl::SetDisconnected() {
  connected_ = false;
}

}  // namespace tor
