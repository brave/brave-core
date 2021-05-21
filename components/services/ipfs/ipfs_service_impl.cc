/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ipfs/ipfs_service_impl.h"

#include <initializer_list>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/process/launch.h"
#include "brave/components/services/ipfs/ipfs_service_utils.h"

namespace {

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

  child_monitor_.reset();
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
      if (callback)
        std::move(callback).Run(false, -1);
      return;
    }
  }
  std::string data;
  if (!base::ReadFileToString(config_path, &data)) {
    VLOG(1) << "Unable to read the ipfs config:" << config_path;
    if (callback)
      std::move(callback).Run(false, -1);
    return;
  }

  std::string updated_config;
  if (!ipfs::UpdateConfigJSON(data, config.get(), &updated_config)) {
    VLOG(1) << "Unable to update the ipfs config:" << config_path;
    if (callback)
      std::move(callback).Run(false, -1);
    return;
  }
  if (!base::WriteFile(config_path, updated_config)) {
    VLOG(1) << "Unable to write the ipfs config:" << config_path;
    if (callback)
      std::move(callback).Run(false, -1);
    return;
  }

  std::initializer_list<std::initializer_list<std::string>> config_args = {
      {"shutdown"},  // Cleanup left-over daemon process.
  };

  for (auto args : config_args) {
    if (!LaunchProcessAndExit(config->binary_path, args, options)) {
      std::move(callback).Run(false, -1);
      return;
    }
  }

  child_monitor_ = std::make_unique<brave::ChildProcessMonitor>();

  // Launch IPFS daemon.
  base::CommandLine args(config->binary_path);
  args.AppendArg("daemon");
  args.AppendArg("--migrate=true");
  args.AppendArg("--enable-gc");
  args.AppendArg("--routing=dhtclient");
  base::Process ipfs_process = base::LaunchProcess(args, options);
  bool result = ipfs_process.IsValid();

  if (callback)
    std::move(callback).Run(result, ipfs_process.Pid());

  // No need to proceed if we fail to launch the daemon, Shutdown will be
  // called after IPFS service in browser process received failed result.
  if (!result) {
    return;
  }

  child_monitor_->Start(std::move(ipfs_process),
                        base::BindOnce(&IpfsServiceImpl::OnChildCrash,
                                       weak_ptr_factory_.GetWeakPtr()));
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
  if (receiver_.is_bound() && crash_handler_callback_ && !in_shutdown_)
    std::move(crash_handler_callback_).Run(pid);
}

}  // namespace ipfs
