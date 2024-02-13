/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

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
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/widevine/static_buildflags.h"
#include "components/component_updater/component_updater_service.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/raw_ref.h"
#include "base/metrics/field_trial_params.h"
#include "base/sequence_checker.h"
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
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/zlib/google/zip.h"
#include "url/gurl.h"

using update_client::CrxInstaller;

#endif

#if !BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

namespace component_updater {

void RegisterWidevineCdmComponent(ComponentUpdateService* cus,
                                  base::OnceClosure callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!IsWidevineEnabled()) {
    return;
  }
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WidevineCdmComponentInstallerPolicy>());
  installer->Register(cus, std::move(callback));
}

}  // namespace component_updater

#else  // !BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

namespace {

// This timeout should be chosen so that downloading the above URL does not
// exceed it. The download is 7.4 MB, which in 60s equates to 1 Mbps. Netflix
// needs 3 Mbps at a minimum and recommends 25 Mbps for high-quality streams.
// So 1 Mbps seems like a conservative number.
constexpr int kDownloadRequestTimeoutSecs = 60;

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
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

// Store external URLs in feature parameters so we can quickly update them with
// Griffin without having to release a new version of the browser.
BASE_DECLARE_FEATURE(kBraveWidevineArm64DllFix);
constexpr base::FeatureParam<std::string> kBraveWidevineArm64DllUrl{
    &kBraveWidevineArm64DllFix, "widevine_arm64_dll_url",
    "https://dl.google.com/widevine-cdm/4.10.2710.0-win-arm64.zip"};
const base::FeatureParam<std::string> kBraveWidevineArm64DllTemplateUrl{
    &kBraveWidevineArm64DllFix, "widevine_arm64_dll_template_url",
    "https://dl.google.com/widevine-cdm/{version}-win-arm64.zip"};
BASE_FEATURE(kBraveWidevineArm64DllFix,
             "BraveWidevineArm64DllFix",
             base::FEATURE_ENABLED_BY_DEFAULT);

std::string GetArm64DllZipUrl(const std::string& version) {
  std::string result = kBraveWidevineArm64DllTemplateUrl.Get();
  base::ReplaceFirstSubstringAfterOffset(&result, 0, "{version}", version);
  return result;
}

}  // namespace

namespace component_updater {

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
  CrxInstaller::Result InstallArm64Dll(const GURL& zip_url,
                                       const base::FilePath& install_dir);
  bool IsHttp404(CrxInstaller::Result);
  std::unique_ptr<network::PendingSharedURLLoaderFactory>
      pending_url_loader_factory_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

class WidevineArm64DllInstaller
    : public base::RefCountedThreadSafe<WidevineArm64DllInstaller> {
 public:
  explicit WidevineArm64DllInstaller(const GURL& zip_url,
                                     const base::FilePath& install_dir);
  void Start(std::unique_ptr<network::PendingSharedURLLoaderFactory>
                 pending_url_loader_factory);
  CrxInstaller::Result WaitForCompletion();

 protected:
  virtual ~WidevineArm64DllInstaller() = default;

 private:
  friend class base::RefCountedThreadSafe<WidevineArm64DllInstaller>;

  void OnArm64DllDownloadComplete(base::FilePath zip_path);
  bool ExtractArm64Dll(base::FilePath zip_path);
  bool AddArm64ArchToManifest();

  const GURL zip_url_;
  const raw_ref<const base::FilePath> install_dir_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  base::WaitableEvent installed_;
  CrxInstaller::Result result_;

  SEQUENCE_CHECKER(sequence_checker_);
};

WidevineCdmComponentInstallerPolicy::WidevineCdmComponentInstallerPolicy(
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory)
    : pending_url_loader_factory_(std::move(pending_url_loader_factory)),
      task_runner_(base::ThreadPool::CreateSingleThreadTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
          base::SingleThreadTaskRunnerThreadMode::DEDICATED)) {}

CrxInstaller::Result WidevineCdmComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  // It would be nice to call the super implementation here. But it is private
  // and (at the time of this writing) a no-op anyways.
  if (base::DirectoryExists(media::GetPlatformSpecificDirectory(install_dir))) {
    LOG(WARNING) << "It seems upstream now supports Widevine on Arm64. "
                    "Consider removing our WIDEVINE_ARM64_DLL_FIX.";
    return CrxInstaller::Result(0);
  }
  const std::string* version = manifest.FindString("version");
  if (!version) {
    return CrxInstaller::Result(update_client::InstallError::INVALID_VERSION);
  }
  // First try to guess a download URL that matches the component version. This
  // usually works and ensures that the component and DLL versions match, even
  // when upstream updates the component.
  const GURL guessed_url = GURL(GetArm64DllZipUrl(*version));
  CrxInstaller::Result result = InstallArm64Dll(guessed_url, install_dir);
  if (IsHttp404(result)) {
    // Our guess failed. Fall back to a DLL version that is known to exist.
    const GURL fallback_url = GURL(kBraveWidevineArm64DllUrl.Get());
    LOG(WARNING) << "Guessed Widevine Arm64 ALL URL " << guessed_url
                 << " does not exist. Falling back to " << fallback_url
                 << ", which should exist but may not be compatible.";
    return InstallArm64Dll(fallback_url, install_dir);
  }
  return result;
}

CrxInstaller::Result WidevineCdmComponentInstallerPolicy::InstallArm64Dll(
    const GURL& zip_url,
    const base::FilePath& install_dir) {
  scoped_refptr<WidevineArm64DllInstaller> installer =
      base::MakeRefCounted<WidevineArm64DllInstaller>(zip_url, install_dir);
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      network::SharedURLLoaderFactory::Create(
          std::move(pending_url_loader_factory_));
  // Restore pending_url_loader_factory_ for future invocations of this method:
  pending_url_loader_factory_ = url_loader_factory->Clone();
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WidevineArm64DllInstaller::Start, installer,
                                url_loader_factory->Clone()));
  return installer->WaitForCompletion();
}

bool WidevineCdmComponentInstallerPolicy::IsHttp404(
    CrxInstaller::Result result) {
  return result.error ==
             static_cast<int>(update_client::InstallError::CUSTOM_ERROR_BASE) +
                 static_cast<int>(net::ERR_HTTP_RESPONSE_CODE_FAILURE) &&
         result.extended_error == 404;
}

WidevineArm64DllInstaller::WidevineArm64DllInstaller(
    const GURL& zip_url,
    const base::FilePath& install_dir)
    : zip_url_(zip_url), install_dir_(install_dir) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

void WidevineArm64DllInstaller::Start(
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = zip_url_;
  loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                             kTrafficAnnotation);
  loader_->SetTimeoutDuration(base::Seconds(kDownloadRequestTimeoutSecs));
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      network::SharedURLLoaderFactory::Create(
          std::move(pending_url_loader_factory));
  VLOG(2) << "Downloading " << zip_url_ << ".";
  loader_->DownloadToTempFile(
      url_loader_factory.get(),
      base::BindOnce(&WidevineArm64DllInstaller::OnArm64DllDownloadComplete,
                     this));
}

CrxInstaller::Result WidevineArm64DllInstaller::WaitForCompletion() {
  // Blocking the thread with a wait is pretty nasty. Unfortunately, we have no
  // other choice: Upstream's implementation of OnCustomInstall is synchronous
  // and making it asynchronous via a callback would require too many changes.
  // At least, upstream guarantees that the thread is blocking. Furthermore,
  // the Widevine component doesn't get installed / updated too often so the
  // effects are limited. We use a timeout when downloading the Arm64 DLL to
  // make sure that we do not block the thread forever.
  base::ScopedAllowBaseSyncPrimitives allow_wait;
  installed_.Wait();
  return result_;
}

void WidevineArm64DllInstaller::OnArm64DllDownloadComplete(
    base::FilePath zip_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (zip_path.empty()) {
    net::Error error = net::Error(loader_->NetError());
    VLOG(1) << "Arm64 DLL download failed. Error: "
            << net::ErrorToShortString(error);
    int extended_error = 0;
    if (loader_->ResponseInfo()) {
      auto headers_list = loader_->ResponseInfo()->headers;
      if (headers_list) {
        extended_error = headers_list->response_code();
      }
    }
    result_ = update_client::ToInstallerResult(error, extended_error);
  } else {
    VLOG(2) << "Arm64 DLL download succeeded.";
    bool success = ExtractArm64Dll(zip_path) && AddArm64ArchToManifest();
    if (!success) {
      result_ =
          CrxInstaller::Result(update_client::InstallError::GENERIC_ERROR);
    }
  }
  // The loader needs to be destroyed on the current sequence. We therefore
  // need to do it here, and cannot wait until the destructor does it.
  loader_.reset();
  installed_.Signal();
}

bool WidevineArm64DllInstaller::ExtractArm64Dll(base::FilePath zip_path) {
  VLOG(2) << "Extracting Arm64 DLL.";
  base::FilePath arm64_directory =
      media::GetPlatformSpecificDirectory(*install_dir_);
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
bool WidevineArm64DllInstaller::AddArm64ArchToManifest() {
  VLOG(2) << "Adding Arm64 to manifest.json.";
  base::FilePath manifest_path = install_dir_->AppendASCII("manifest.json");

  std::string json_content;
  if (!base::ReadFileToString(manifest_path, &json_content)) {
    LOG(ERROR) << "Failed to read file: " << manifest_path;
    return false;
  }

  std::optional<base::Value> root = base::JSONReader::Read(json_content);
  if (!root) {
    LOG(ERROR) << "Failed to parse JSON.";
    return false;
  }

  base::Value::Dict* root_dict = root->GetIfDict();
  if (!root_dict) {
    LOG(ERROR) << "Manifest is not a dictionary.";
    return false;
  }

  base::Value* accept_arch = root_dict->Find("accept_arch");
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

void RegisterWidevineCdmComponent(
    ComponentUpdateService* cus,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    base::OnceClosure callback) {
  VLOG(1) << "RegisterWidevineCdmComponent";
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!IsWidevineEnabled()) {
    return;
  }
  CHECK(url_loader_factory);
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WidevineCdmComponentInstallerPolicy>(
          url_loader_factory->Clone()));
  installer->Register(cus, std::move(callback));
}

}  // namespace component_updater

#endif  // !BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
