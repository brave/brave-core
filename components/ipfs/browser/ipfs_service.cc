/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/browser/ipfs_service.h"

#include <utility>

#include "base/path_service.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/service_process_host.h"

namespace ipfs {

IpfsService::IpfsService(content::BrowserContext* context) {
  g_brave_browser_process->ipfs_client_updater()->AddObserver(this);
  OnExecutableReady(GetIpfsExecutablePath());
}

IpfsService::~IpfsService() = default;

base::FilePath IpfsService::GetIpfsExecutablePath() {
  return g_brave_browser_process->ipfs_client_updater()->GetExecutablePath();
}

void IpfsService::OnExecutableReady(const base::FilePath& path) {
  if (path.empty())
    return;

  g_brave_browser_process->ipfs_client_updater()->RemoveObserver(this);
  LaunchIfNotRunning(path);
}

void IpfsService::LaunchIfNotRunning(const base::FilePath& executable_path) {
  if (ipfs_service_.is_bound())
    return;

  content::ServiceProcessHost::Launch(
      ipfs_service_.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
        .WithDisplayName(IDS_UTILITY_PROCESS_IPFS_NAME)
        .WithSandboxType(service_manager::SandboxType::kNoSandbox)
        .Pass());

  ipfs_service_.set_disconnect_handler(
      base::BindOnce(
        &IpfsService::OnIpfsCrashed,
        base::Unretained(this)));

  ipfs_service_->SetCrashHandler(
      base::Bind(&IpfsService::OnIpfsDaemonCrashed,
        base::Unretained(this)));

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());

  base::FilePath data_root_path =
    user_data_dir.Append(FILE_PATH_LITERAL("ipfs"));
  base::FilePath config_path =
    data_root_path.Append(FILE_PATH_LITERAL("config"));

  auto config = mojom::IpfsConfig::New(
      executable_path,
      config_path,
      data_root_path);

  ipfs_service_->Launch(
      std::move(config),
      base::Bind(&IpfsService::OnIpfsLaunched,
        base::Unretained(this)));
}

void IpfsService::OnIpfsCrashed() {
  VLOG(0) << "IPFS utility process crashed";
  Shutdown();
}

void IpfsService::OnIpfsDaemonCrashed(int64_t pid) {
  VLOG(0) << "IPFS daemon crashed";
  Shutdown();
}

void IpfsService::OnIpfsLaunched(bool result, int64_t pid) {
  if (result) {
    ipfs_pid_ = pid;
  } else {
    VLOG(0) << "Failed to launch IPFS";
    Shutdown();
  }
}

void IpfsService::Shutdown() {
  if (ipfs_service_.is_bound()) {
    ipfs_service_->Shutdown();
  }

  ipfs_service_.reset();
  ipfs_pid_ = -1;
}

}  // namespace ipfs
