/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/child_process_monitor/child_process_monitor.h"

#if defined(OS_POSIX)
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(OS_WIN)
#include <windows.h>
#endif

#include <utility>

#include "base/bind_post_task.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/single_thread_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace brave {

namespace {

#if defined(OS_POSIX)
int pipehack[2];

void SIGCHLDHandler(int signo) {
  int error = errno;
  char ch = 0;
  (void)write(pipehack[1], &ch, 1);
  errno = error;
}

#if defined(OS_MAC)
void SetupFD(int fd) {
  int flags;
  if ((flags = fcntl(fd, F_GETFD)) == -1)
    VLOG(0) << "get fd flags errno:" << errno;
  flags |= FD_CLOEXEC;
  if (fcntl(fd, F_SETFD, flags) == -1)
    VLOG(0) << "set fd flags errno:" << errno;
}
#endif

void SetupPipeHack() {
#if defined(OS_MAC)
  if (pipe(pipehack) == -1)
    VLOG(0) << "pipehack errno:" << errno;
  SetupFD(pipehack[0]);
  SetupFD(pipehack[1]);
#else
  if (pipe2(pipehack, O_CLOEXEC) == -1)
    VLOG(0) << "pipehack errno:" << errno;
#endif

  int flags;
  if ((flags = fcntl(pipehack[1], F_GETFL)) == -1)
    VLOG(0) << "get flags errno:" << errno;
  // Nonblock write end on SIGCHLD handler which will notify monitor thread
  // by sending one byte to pipe whose read end is blocked and wait for
  // SIGCHLD to arrives to avoid busy reading
  flags |= O_NONBLOCK;
  if (fcntl(pipehack[1], F_SETFL, flags) == -1)
    VLOG(0) << "set flags errno:" << errno;

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGCHLDHandler;
  action.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &action, NULL);
}

void TearDownPipeHack() {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_DFL;
  action.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &action, NULL);
  close(pipehack[0]);
  close(pipehack[1]);
}
#endif

void MonitorChild(base::ProcessHandle p_handle,
                  base::OnceCallback<void(base::ProcessId)> callback) {
  DCHECK(callback);
#if defined(OS_POSIX)
  char buf[PIPE_BUF];

  while (1) {
    if (read(pipehack[0], buf, sizeof(buf)) > 0) {
      pid_t pid;
      int status;

      if ((pid = waitpid(base::GetProcId(p_handle), &status, WNOHANG)) != -1) {
        if (WIFSIGNALED(status)) {
          VLOG(0) << "child(" << pid << ") got terminated by signal "
                  << WTERMSIG(status);
        } else if (WCOREDUMP(status)) {
          VLOG(0) << "child(" << pid << ") coredumped";
        } else if (WIFEXITED(status)) {
          VLOG(0) << "child(" << pid << ") exit (" << WEXITSTATUS(status)
                  << ")";
        }
        std::move(callback).Run(pid);
        break;
      }
    } else {
      // pipes closed
      break;
    }
  }
#elif defined(OS_WIN)
  WaitForSingleObject(p_handle, INFINITE);
  std::move(callback).Run(base::GetProcId(p_handle));
#else
#error unsupported platforms
#endif
}

}  // namespace

ChildProcessMonitor::ChildProcessMonitor()
    : child_monitor_thread_(
          std::make_unique<base::Thread>("child_monitor_thread")) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#if defined(OS_POSIX)
  SetupPipeHack();
#endif
  if (!child_monitor_thread_->Start()) {
    NOTREACHED();
  }
}

ChildProcessMonitor::~ChildProcessMonitor() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#if defined(OS_POSIX)
  TearDownPipeHack();
#endif

  if (child_process_.IsValid()) {
    child_process_.Terminate(0, true);
#if defined(OS_MAC)
    // TODO(https://crbug.com/806451): The Mac implementation currently blocks
    // the calling thread for up to two seconds.
    base::ThreadPool::PostTask(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
        base::BindOnce(&base::EnsureProcessTerminated,
                       std::move(child_process_)));
#else
    base::EnsureProcessTerminated(std::move(child_process_));
#endif
  }
}

void ChildProcessMonitor::Start(
    base::Process child,
    base::OnceCallback<void(base::ProcessId)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  child_process_ = std::move(child);

  DCHECK(child_monitor_thread_);
  child_monitor_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MonitorChild, child_process_.Handle(),
          base::BindPostTask(base::SequencedTaskRunnerHandle::Get(),
                             base::BindOnce(&ChildProcessMonitor::OnChildCrash,
                                            weak_ptr_factory_.GetWeakPtr(),
                                            std::move(callback)))));
}

void ChildProcessMonitor::OnChildCrash(
    base::OnceCallback<void(base::ProcessId)> callback,
    base::ProcessId pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(callback);

  child_process_.Close();
  std::move(callback).Run(pid);
}

}  // namespace brave
