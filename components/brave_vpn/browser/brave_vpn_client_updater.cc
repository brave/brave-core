// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_vpn/browser/brave_vpn_client_updater.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/task_runner.h"
#include "base/task/thread_pool.h"
#include "components/component_updater/component_updater_service.h"

namespace brave_vpn {

std::string BraveVpnClientUpdater::g_vpn_client_component_id_(
    kBraveVpnClientComponentId);
std::string BraveVpnClientUpdater::g_vpn_client_component_base64_public_key_(
    kBraveVpnClientComponentBase64PublicKey);

BraveVpnClientUpdater::BraveVpnClientUpdater(
    BraveComponent::Delegate* delegate,
    const base::FilePath& user_data_dir)
    : BraveComponent(delegate),
      task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      registered_(false),
      user_data_dir_(user_data_dir),
      weak_ptr_factory_(this) {}

BraveVpnClientUpdater::~BraveVpnClientUpdater() = default;

void BraveVpnClientUpdater::Register() {
  if (registered_) {
    return;
  }

  BraveComponent::Register(kBraveVpnClientComponentName,
                           g_vpn_client_component_id_,
                           g_vpn_client_component_base64_public_key_);
  if (!updater_observer_.IsObservingSource(this)) {
    updater_observer_.Observe(this);
  }
  registered_ = true;
}

namespace {

base::FilePath InitExecutablePath(const base::FilePath& install_dir) {
  base::FilePath executable_path;

  // TODO(bsclifton): perform the install / setup here
  //
  // commented out code below is left as-is while this is fleshed out
  // code is from brave/components/ipfs/brave_ipfs_client_updater.cc
  //-------------------------------------------------------------------------
  //   base::FileEnumerator traversal(install_dir, false,
  //                                  base::FileEnumerator::FILES,
  //                                  FILE_PATH_LITERAL("go-ipfs_v*"));
  //   for (base::FilePath current = traversal.Next(); !current.empty();
  //        current = traversal.Next()) {
  //     base::FileEnumerator::FileInfo file_info = traversal.GetInfo();
  //     if (!ipfs::IsValidNodeFilename(file_info.GetName().MaybeAsASCII()))
  //       continue;
  //     executable_path = current;
  //     break;
  //   }
  //
  //   if (executable_path.empty()) {
  //     LOG(ERROR) << "Failed to locate Ipfs client executable in "
  //                << install_dir.value().c_str();
  //     return base::FilePath();
  //   }
  //
  // #if BUILDFLAG(IS_POSIX)
  //   // Ensure that Ipfs client executable has appropriate file
  //   // permissions, as CRX unzipping does not preserve them.
  //   // See https://crbug.com/555011
  //   if (!base::SetPosixFilePermissions(executable_path, 0755)) {
  //     LOG(ERROR) << "Failed to set executable permission on "
  //                << executable_path.value().c_str();
  //     return base::FilePath();
  //   }
  // #endif  // BUILDFLAG(IS_POSIX)
  //-------------------------------------------------------------------------

  return executable_path;
}

void DeleteDir(const base::FilePath& path) {
  base::DeletePathRecursively(path);
}

}  // namespace

void BraveVpnClientUpdater::SetExecutablePath(const base::FilePath& path) {
  executable_path_ = path;
  for (Observer& observer : observers_) {
    observer.OnExecutableReady(path);
  }
}

base::FilePath BraveVpnClientUpdater::GetExecutablePath() const {
  return executable_path_;
}

void BraveVpnClientUpdater::OnEvent(Events event, const std::string& id) {
  if (id != g_vpn_client_component_id_) {
    return;
  }
  if (event == Events::COMPONENT_UPDATE_ERROR) {
    registered_ = false;
  }
  for (Observer& observer : observers_) {
    observer.OnInstallationEvent(event);
  }
}

void BraveVpnClientUpdater::OnComponentReady(const std::string& component_id,
                                             const base::FilePath& install_dir,
                                             const std::string& manifest) {
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&InitExecutablePath, install_dir),
      base::BindOnce(&BraveVpnClientUpdater::SetExecutablePath,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveVpnClientUpdater::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVpnClientUpdater::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVpnClientUpdater::Cleanup() {
  DCHECK(!user_data_dir_.empty());
  base::FilePath vpn_client_component_dir =
      user_data_dir_.AppendASCII(g_vpn_client_component_id_);
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&DeleteDir, vpn_client_component_dir));
}

// static
void BraveVpnClientUpdater::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_vpn_client_component_id_ = component_id;
  g_vpn_client_component_base64_public_key_ = component_base64_public_key;
}

///////////////////////////////////////////////////////////////////////////////

// The Brave VPN client extension factory.
std::unique_ptr<BraveVpnClientUpdater> BraveVpnClientUpdaterFactory(
    BraveComponent::Delegate* delegate,
    const base::FilePath& user_data_dir) {
  return std::make_unique<BraveVpnClientUpdater>(delegate, user_data_dir);
}

}  // namespace brave_vpn
