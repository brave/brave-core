/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_ipfs_client_updater.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "third_party/re2/src/re2/re2.h"

namespace extensions {

std::string BraveIpfsClientUpdater::g_ipfs_client_component_id_(
    kIpfsClientComponentId);
std::string BraveIpfsClientUpdater::g_ipfs_client_component_base64_public_key_(
    kIpfsClientComponentBase64PublicKey);

BraveIpfsClientUpdater::BraveIpfsClientUpdater()
    : task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      registered_(false) {
}

BraveIpfsClientUpdater::~BraveIpfsClientUpdater() {
}

void BraveIpfsClientUpdater::Register() {
  if (registered_)
    return;

  BraveComponentExtension::Register(kIpfsClientComponentName,
                                    g_ipfs_client_component_id_,
                                    g_ipfs_client_component_base64_public_key_);
  registered_ = true;
}

base::FilePath BraveIpfsClientUpdater::GetExecutablePath() const {
  return executable_path_;
}

void BraveIpfsClientUpdater::InitExecutablePath(
    const base::FilePath& install_dir) {
  base::FilePath executable_path;
  base::FileEnumerator traversal(install_dir, false,
                                 base::FileEnumerator::FILES,
                                 FILE_PATH_LITERAL("ipfs-*"));
  for (base::FilePath current = traversal.Next(); !current.empty();
       current = traversal.Next()) {
    base::FileEnumerator::FileInfo file_info = traversal.GetInfo();
    if (!RE2::FullMatch(file_info.GetName().MaybeAsASCII(),
                        "ipfs-client"))
      continue;
    executable_path = current;
    break;
  }

  if (executable_path.empty()) {
    LOG(ERROR) << "Failed to locate Ipfs client executable in "
               << install_dir.value().c_str();
    return;
  }

#if defined(OS_POSIX)
  // Ensure that Ipfs client executable has appropriate file
  // permissions, as CRX unzipping does not preserve them.
  // See https://crbug.com/555011
  if (!base::SetPosixFilePermissions(executable_path, 0755)) {
    LOG(ERROR) << "Failed to set executable permission on "
               << executable_path.value().c_str();
    return;
  }
#endif // defined(OS_POSIX)

  executable_path_ = executable_path;
}

void BraveIpfsClientUpdater::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir) {
  GetTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&BraveIpfsClientUpdater::InitExecutablePath,
                            base::Unretained(this), install_dir));
}

// static
void BraveIpfsClientUpdater::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_ipfs_client_component_id_ = component_id;
  g_ipfs_client_component_base64_public_key_ = component_base64_public_key;
}

///////////////////////////////////////////////////////////////////////////////

// The Brave Ipfs client extension factory.
std::unique_ptr<BraveIpfsClientUpdater> BraveIpfsClientUpdaterFactory() {
  return std::make_unique<BraveIpfsClientUpdater>();
}

}  // namespace extensions
