/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/tor/tor_launcher_impl.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"

namespace tor {

TorLauncherImpl::TorLauncherImpl(
    mojo::PendingReceiver<mojom::TorLauncher> receiver)
    : child_monitor_(std::make_unique<brave::ChildProcessMonitor>()),
      receiver_(this, std::move(receiver)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receiver_.set_disconnect_handler(base::BindOnce(
      &TorLauncherImpl::Cleanup, weak_ptr_factory_.GetWeakPtr()));
}

void TorLauncherImpl::Cleanup() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (in_shutdown_)
    return;
  in_shutdown_ = true;

  // Delete watch folder every time that Tor is terminated
  base::DeletePathRecursively(tor_watch_path_);
  child_monitor_.reset();
}

TorLauncherImpl::~TorLauncherImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Cleanup();
}

void TorLauncherImpl::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Cleanup();
}

void TorLauncherImpl::Launch(mojom::TorConfigPtr config,
                             LaunchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (in_shutdown_) {
    if (callback)
      std::move(callback).Run(false, -1);
    return;
  }
  base::CommandLine args(config->binary_path);
  args.AppendArg("--ignore-missing-torrc");
  args.AppendArg("-f");
  args.AppendArg("/nonexistent");
  args.AppendArg("--defaults-torrc");
  args.AppendArg("/nonexistent");
  args.AppendArg("--SocksPort");
  args.AppendArg("auto");
  base::FilePath tor_data_path = config->tor_data_path;
  if (!tor_data_path.empty()) {
    if (!base::DirectoryExists(tor_data_path))
      base::CreateDirectory(tor_data_path);
    args.AppendArg("--DataDirectory");
    args.AppendArgPath(tor_data_path);
  }
  args.AppendArg("--__OwningControllerProcess");
  args.AppendArg(base::NumberToString(base::Process::Current().Pid()));
  tor_watch_path_ = config->tor_watch_path;
  if (!tor_watch_path_.empty()) {
    if (!base::DirectoryExists(tor_watch_path_))
      base::CreateDirectory(tor_watch_path_);
    args.AppendArg("--pidfile");
    args.AppendArgPath(tor_watch_path_.AppendASCII("tor.pid"));
    args.AppendArg("--controlport");
    args.AppendArg("auto");
    args.AppendArg("--controlportwritetofile");
    args.AppendArgPath(tor_watch_path_.AppendASCII("controlport"));
    args.AppendArg("--cookieauthentication");
    args.AppendArg("1");
    args.AppendArg("--cookieauthfile");
    args.AppendArgPath(tor_watch_path_.AppendASCII("control_auth_cookie"));
  }

  base::LaunchOptions launchopts;
#if defined(OS_LINUX)
  launchopts.kill_on_parent_death = true;
#endif
#if defined(OS_WIN)
  launchopts.start_hidden = true;
#endif
  base::Process tor_process = base::LaunchProcess(args, launchopts);

  bool result = tor_process.IsValid();

  if (callback)
    std::move(callback).Run(result, tor_process.Pid());

  child_monitor_->Start(std::move(tor_process),
                        base::BindOnce(&TorLauncherImpl::OnChildCrash,
                                       weak_ptr_factory_.GetWeakPtr()));
}

void TorLauncherImpl::SetCrashHandler(SetCrashHandlerCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  crash_handler_callback_ = std::move(callback);
}

void TorLauncherImpl::OnChildCrash(base::ProcessId pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (receiver_.is_bound() && crash_handler_callback_ && !in_shutdown_)
    std::move(crash_handler_callback_).Run(pid);
}

}  // namespace tor
