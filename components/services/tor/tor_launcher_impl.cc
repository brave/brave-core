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
#include "brave/components/tor/constants.h"
#include "build/build_config.h"

namespace tor {

namespace {

// A utiltiy function to create the missing directories that are used by the
// launcher.
base::FilePath CreateIfNotExists(const base::FilePath& path) {
  if (!base::DirectoryExists(path)) {
    CHECK(base::CreateDirectory(path));
  }
  return path;
}

}  // namespace

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
  base::CommandLine args(
      GetClientExecutablePath(config->install_dir, config->executable));
  args.AppendArg("--ignore-missing-torrc");

  auto torrc_path = GetTorRcPath(config->install_dir);
  args.AppendArg("-f");
  args.AppendArgPath(torrc_path);
  args.AppendArg("--defaults-torrc");
  args.AppendArgPath(torrc_path);

  args.AppendArg("--DataDirectory");
  args.AppendArgPath(CreateIfNotExists(GetTorDataPath()));
  args.AppendArg("--__OwningControllerProcess");
  args.AppendArg(base::NumberToString(base::Process::Current().Pid()));
  tor_watch_path_ = CreateIfNotExists(GetTorWatchPath());
  args.AppendArg("--pidfile");
  args.AppendArgPath(tor_watch_path_.AppendASCII("tor.pid"));
  args.AppendArg("--controlportwritetofile");
  args.AppendArgPath(tor_watch_path_.AppendASCII("controlport"));
  args.AppendArg("--cookieauthfile");
  args.AppendArgPath(tor_watch_path_.AppendASCII("control_auth_cookie"));

  base::LaunchOptions launchopts;
#if BUILDFLAG(IS_LINUX)
  launchopts.kill_on_parent_death = true;
#endif
#if BUILDFLAG(IS_WIN)
  launchopts.start_hidden = true;
#endif
  // This line is necessary as the paths for tor_snowflake and tor_obfs4 are set
  // up relative to this binary.
  launchopts.current_directory = args.GetProgram().DirName();
  base::Process tor_process = base::LaunchProcess(args, launchopts);

  if (callback) {
    std::move(callback).Run(tor_process.IsValid(), tor_process.Pid());
  }

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
