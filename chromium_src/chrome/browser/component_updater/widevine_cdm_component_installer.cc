/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define WidevineCdmComponentInstallerPolicy \
  WidevineCdmComponentInstallerPolicy_ChromiumImpl
#endif

#define RegisterWidevineCdmComponent RegisterWidevineCdmComponent_ChromiumImpl
#include "src/chrome/browser/component_updater/widevine_cdm_component_installer.cc"
#undef RegisterWidevineCdmComponent

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#undef WidevineCdmComponentInstallerPolicy
#endif

#include "base/check.h"
#include "base/functional/callback.h"
#include "brave/browser/widevine/buildflags.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "components/component_updater/component_updater_service.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/update_client/task_traits.h"
#include "components/update_client/utils.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/zlib/google/zip.h"
#include "url/gurl.h"

#endif

using update_client::CrxInstaller;

#if !BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

namespace component_updater {

void RegisterWidevineCdmComponent(ComponentUpdateService* cus,
                                  base::OnceCallback<void()> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!IsWidevineOptedIn()) {
    return;
  }
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WidevineCdmComponentInstallerPolicy>());
  installer->Register(cus, std::move(callback));
}

}  // namespace component_updater

#else  // !BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

namespace {

const char kArm64DllUrl[] =
    "https://dl.google.com/widevine-cdm/4.10.2557.0-win-arm64.zip";

// This timeout should be chosen so that downloading the above URL does not
// exceed it. The download is 7.4 MB, which in 60s equates to 1 Mbps. Netflix
// needs 3 Mbps at a minimum and recommends 25 Mbps for high-quality streams.
// So 1 Mbps seems like a conservative number.
const int kDownloadRequestTimeoutSecs = 60;
const int kOverallDownloadTimeoutSecs = kDownloadRequestTimeoutSecs + 1;

const net::NetworkTrafficAnnotationTag traffic_annotation =
    net::DefineNetworkTrafficAnnotation("widevine_updater", R"(
        semantics {
          sender: "Widevine Component Updater"
          description:
            "This network module is used by the Widevine component updater. "
            "The component updater is responsible for updating code and data "
            "modules for playing DRM-protected content. The modules are "
            "updated on cycles independent of the Chrome release tracks. "
            "The Widevine component updater runs in the browser process and "
            "downloads the latest version of the component from Google's "
            "servers."
          trigger: "Manual or automatic software updates."
          data: "None."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature is off by default and can be overridden by users."
          policy_exception_justification:
            "No policy provided because the user is asked for consent before "
            "the feature is enabled."
        })");

}  // namespace

namespace component_updater {

void RegisterWidevineCdmComponent(
    ComponentUpdateService* cus,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    base::OnceCallback<void()> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!IsWidevineOptedIn()) {
    return;
  }
  CHECK(url_loader_factory);
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WidevineCdmComponentInstallerPolicy>(
          url_loader_factory->Clone()));
  installer->Register(cus, std::move(callback));
}

class WidevineCdmComponentInstallerPolicy
    : public WidevineCdmComponentInstallerPolicy_ChromiumImpl {
 public:
  WidevineCdmComponentInstallerPolicy(
      std::unique_ptr<network::PendingSharedURLLoaderFactory>
          pending_url_loader_factory);

 private:
  CrxInstaller::Result OnCustomInstall(
      const base::Value::Dict& manifest,
      const base::FilePath& install_dir) override;
  void DownloadArm64Dll(const base::FilePath& install_dir);
  void OnArm64DllDownloadComplete(const base::FilePath& install_dir,
                                  base::FilePath zip_path);
  bool ExtractArm64Dll(const base::FilePath& install_dir,
                       base::FilePath zip_path);
  bool AddArm64ArchToManifest(const base::FilePath& install_dir);
  void ResetForNextUpdate();

  std::unique_ptr<network::PendingSharedURLLoaderFactory>
      pending_url_loader_factory_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WaitableEvent installed_;
  CrxInstaller::Result result_;
  base::WeakPtrFactory<WidevineCdmComponentInstallerPolicy> weak_ptr_factory_{
      this};
};

WidevineCdmComponentInstallerPolicy::WidevineCdmComponentInstallerPolicy(
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory)
    : pending_url_loader_factory_(std::move(pending_url_loader_factory)),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          update_client::kTaskTraits)) {}

CrxInstaller::Result WidevineCdmComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  // It would be nice to call the super implementation here. But it is private
  // and (at the time of this writing) a no-op anyways.
  if (base::DirectoryExists(GetPlatformDirectory(install_dir))) {
    LOG(WARNING) << "It seems upstream now supports Widevine on Arm64. "
                    "Consider removing our WIDEVINE_ARM64_DLL_FIX.";
    return CrxInstaller::Result(0);
  }
  // Reset the result after a potential previous invocation:
  result_ = CrxInstaller::Result(0);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WidevineCdmComponentInstallerPolicy::DownloadArm64Dll,
                     weak_ptr_factory_.GetWeakPtr(), install_dir));
  // Blocking the thread with a wait is pretty nasty. Unfortunately, we have no
  // other choice: Upstream's implementation of OnCustomInstall is synchronous
  // and making it asynchronous via a callback would require too many changes.
  // At least, upstream guarantees that the thread is blocking. Furthermore,
  // the Widevine component doesn't get installed / updated too often so the
  // effects are limited. Finally, we use TimedWait(...) instead of Wait(...) to
  // really make sure that we do not block the thread forever.
  base::ScopedAllowBaseSyncPrimitives allow_wait;
  bool success =
      installed_.TimedWait(base::Seconds(kOverallDownloadTimeoutSecs));
  ResetForNextUpdate();
  if (!success) {
    LOG(ERROR) << "Arm64 DLL download timeout expired.";
    return CrxInstaller::Result(update_client::InstallError::GENERIC_ERROR);
  }
  return result_;
}

void WidevineCdmComponentInstallerPolicy::DownloadArm64Dll(
    const base::FilePath& install_dir) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kArm64DllUrl);

  // This implementation does not expect to run concurrently.
  CHECK(!loader_);
  loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                             traffic_annotation);
  loader_->SetTimeoutDuration(base::Seconds(kDownloadRequestTimeoutSecs));
  if (!url_loader_factory_) {
    // We are assigning to this field here (instead of, say, the constructor)
    // because it needs to be done on the right sequence.
    url_loader_factory_ = network::SharedURLLoaderFactory::Create(
        std::move(pending_url_loader_factory_));
  }
  loader_->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(
          &WidevineCdmComponentInstallerPolicy::OnArm64DllDownloadComplete,
          weak_ptr_factory_.GetWeakPtr(), install_dir));
}

void WidevineCdmComponentInstallerPolicy::OnArm64DllDownloadComplete(
    const base::FilePath& install_dir,
    base::FilePath zip_path) {
  if (zip_path.empty()) {
    net::Error error = net::Error(loader_->NetError());
    LOG(ERROR) << "Arm64 DLL download failed. Error: "
               << net::ErrorToShortString(error);
    result_ = update_client::ToInstallerResult(error);
  } else {
    bool success = ExtractArm64Dll(install_dir, zip_path) &&
                   AddArm64ArchToManifest(install_dir);
    if (!success) {
      result_ =
          CrxInstaller::Result(update_client::InstallError::GENERIC_ERROR);
    }
  }
  installed_.Signal();
}

bool WidevineCdmComponentInstallerPolicy::ExtractArm64Dll(
    const base::FilePath& install_dir,
    base::FilePath zip_path) {
  base::FilePath arm64_directory = GetPlatformDirectory(install_dir);
  base::File::Error error;
  if (base::CreateDirectoryAndGetError(arm64_directory, &error)) {
    if (!zip::Unzip(zip_path, arm64_directory)) {
      LOG(ERROR) << "Failed to unzip Arm64 DLL.";
      return false;
    }
  } else {
    LOG(ERROR) << "Failed to create " << arm64_directory << ": "
               << base::File::ErrorToString(error);
    return false;
  }
  return true;
}

// Components contain a "manifest.json" file that has a list "accept_arch",
// which lists the supported architectures. The file is checked on browser
// startup when the components are registered. If the browser's architecture is
// not in the list of supported architectures, then the component is
// uninstalled. To prevent this from happening to our WIDEVINE_ARM64_DLL_FIX,
// we need to add "arm64" to the list of supported architectures.
bool WidevineCdmComponentInstallerPolicy::AddArm64ArchToManifest(
    const base::FilePath& install_dir) {
  base::FilePath manifest_path = install_dir.AppendASCII("manifest.json");

  std::string json_content;
  if (!base::ReadFileToString(manifest_path, &json_content)) {
    LOG(ERROR) << "Failed to read file: " << manifest_path;
    return false;
  }

  absl::optional<base::Value> root = base::JSONReader::Read(json_content);
  if (!root) {
    LOG(ERROR) << "Failed to parse JSON.";
    return false;
  }

  const base::Value::Dict* root_dict = root->GetIfDict();
  if (!root_dict) {
    LOG(ERROR) << "Manifest is not a dictionary.";
    return false;
  }

  base::Value* accept_arch = root->FindPath("accept_arch");
  if (!accept_arch) {
    LOG(ERROR) << "Could not find accept_arch field.";
    return false;
  }

  base::Value::List* accept_arch_list = accept_arch->GetIfList();
  if (!accept_arch_list) {
    LOG(ERROR) << "accept_arch is not a list.";
    return false;
  }

  accept_arch_list->Append("arm64");

  std::string new_json_content;
  base::JSONWriter::Write(*root, &new_json_content);

  if (!base::WriteFile(manifest_path, new_json_content)) {
    LOG(ERROR) << "Failed to write file: " << manifest_path;
    return false;
  }

  return true;
}

void WidevineCdmComponentInstallerPolicy::ResetForNextUpdate() {
  loader_.reset();
  installed_.Reset();
}

#endif  // !BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

}  // namespace component_updater
