/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_tor_client_updater.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "third_party/re2/src/re2/re2.h"

using brave_component_updater::BraveComponent;

namespace extensions {

std::string BraveTorClientUpdater::g_tor_client_component_id_(
    kTorClientComponentId);
std::string BraveTorClientUpdater::g_tor_client_component_base64_public_key_(
    kTorClientComponentBase64PublicKey);

BraveTorClientUpdater::BraveTorClientUpdater(BraveComponent::Delegate* delegate)
    : BraveComponent(delegate),
      task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      registered_(false) {
}

BraveTorClientUpdater::~BraveTorClientUpdater() {
}

void BraveTorClientUpdater::Register() {
  if (registered_)
    return;

  BraveComponent::Register(kTorClientComponentName,
                               g_tor_client_component_id_,
                               g_tor_client_component_base64_public_key_);
  registered_ = true;
}

base::FilePath BraveTorClientUpdater::GetExecutablePath() const {
  return executable_path_;
}

void BraveTorClientUpdater::InitExecutablePath(
    const base::FilePath& install_dir) {
  base::FilePath executable_path;
  base::FileEnumerator traversal(install_dir, false,
                                 base::FileEnumerator::FILES,
                                 FILE_PATH_LITERAL("tor-*"));
  for (base::FilePath current = traversal.Next(); !current.empty();
       current = traversal.Next()) {
    base::FileEnumerator::FileInfo file_info = traversal.GetInfo();
    if (!RE2::FullMatch(file_info.GetName().MaybeAsASCII(),
                        "tor-\\d+\\.\\d+\\.\\d+\\.\\d+-\\w+-brave-\\d+"))
      continue;
    executable_path = current;
    break;
  }

  if (executable_path.empty()) {
    LOG(ERROR) << "Failed to locate Tor client executable in "
               << install_dir.value().c_str();
    return;
  }

#if defined(OS_POSIX)
  // Ensure that Tor client executable has appropriate file
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

void BraveTorClientUpdater::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  GetTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&BraveTorClientUpdater::InitExecutablePath,
                            base::Unretained(this), install_dir));
}

// static
void BraveTorClientUpdater::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_tor_client_component_id_ = component_id;
  g_tor_client_component_base64_public_key_ = component_base64_public_key;
}

///////////////////////////////////////////////////////////////////////////////

// The Brave Tor client extension factory.
std::unique_ptr<BraveTorClientUpdater>
BraveTorClientUpdaterFactory(BraveComponent::Delegate* delegate) {
  return std::make_unique<BraveTorClientUpdater>(delegate);
}

}  // namespace extensions
