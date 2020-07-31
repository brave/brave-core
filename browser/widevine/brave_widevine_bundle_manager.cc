/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/brave_widevine_bundle_manager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "base/task/post_task.h"
#include "brave/browser/widevine/brave_widevine_bundle_unzipper.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/common/brave_switches.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/cdm_info.h"
#include "content/public/browser/cdm_registry.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#include "url/gurl.h"
#include "widevine_cdm_version.h"  // NOLINT

namespace {

// Try five times when background update is failed.
const int kMaxBackgroundUpdateRetry = 5;

int GetBackgroundUpdateDelayInMins() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kFastWidevineBundleUpdate) ? 0 : 5;
}

base::Optional<base::FilePath> GetTargetWidevineBundleDir() {
  base::FilePath widevine_cdm_dir;
  if (base::PathService::Get(chrome::DIR_USER_DATA, &widevine_cdm_dir)) {
    widevine_cdm_dir = widevine_cdm_dir.Append(
        FILE_PATH_LITERAL(kWidevineCdmBaseDirectory));
    base::CreateDirectory(widevine_cdm_dir);
    return widevine_cdm_dir;
  }

  return base::Optional<base::FilePath>();
}

void ResetWidevinePrefs() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  SetWidevineOptedIn(false);
  SetWidevineInstalledVersion(
      BraveWidevineBundleManager::kWidevineInvalidVersion);
}

void SetWidevinePrefsAsInstalledState() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  SetWidevineOptedIn(true);
  SetWidevineInstalledVersion(WIDEVINE_CDM_VERSION_STRING);
}

}  // namespace

// static
char BraveWidevineBundleManager::kWidevineInvalidVersion[] = "";

BraveWidevineBundleManager::BraveWidevineBundleManager() : weak_factory_(this) {
}

BraveWidevineBundleManager::~BraveWidevineBundleManager() {
}

void BraveWidevineBundleManager::InstallWidevineBundle(
    DoneCallback done_callback,
    bool user_gesture) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << __func__ << (user_gesture ? ": Install widevine bundle"
                                        : ": Update widevine bundle");
  DCHECK(!needs_restart());
  DCHECK(startup_checked_);

  done_callback_ = std::move(done_callback);
  set_in_progress(true);

  DownloadWidevineBundle(
      GURL(WIDEVINE_CDM_DOWNLOAD_URL_STRING),
      base::BindOnce(&BraveWidevineBundleManager::OnBundleDownloaded,
                     base::Unretained(this)));
  DeleteDeprecatedWidevineCdmLib();
}

void BraveWidevineBundleManager::DownloadWidevineBundle(
    const GURL& bundle_zipfile_url,
    network::SimpleURLLoader::DownloadToFileCompleteCallback callback) {
  if (is_test_) return;

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("widevine_bundle_downloader", R"(
        semantics {
          sender:
            "Brave Widevine Bundle Manager"
          description:
            "Download widevine cdm pkg"
          trigger:
            "When user accpets the use of widevine or update is started"
          data: "Widevine cdm library package"
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature can be disabled by disabling widevine in linux"
          policy_exception_justification:
            "Not implemented."
        })");

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = bundle_zipfile_url;
  bundle_loader_ =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
  bundle_loader_->DownloadToTempFile(
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory(),
      std::move(callback));
}

void BraveWidevineBundleManager::OnBundleDownloaded(
    base::FilePath tmp_bundle_zip_file_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << __func__;

  if (tmp_bundle_zip_file_path.empty()) {
    InstallDone("bundle file download failed");
    return;
  }

  base::PostTaskAndReplyWithResult(
      file_task_runner().get(),
      FROM_HERE,
      base::BindOnce(&GetTargetWidevineBundleDir),
      base::BindOnce(&BraveWidevineBundleManager::OnGetTargetWidevineBundleDir,
                     weak_factory_.GetWeakPtr(),
                     tmp_bundle_zip_file_path));
}

void BraveWidevineBundleManager::OnGetTargetWidevineBundleDir(
    const base::FilePath& tmp_bundle_zip_file_path,
    base::Optional<base::FilePath> target_bundle_dir) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << __func__;

  if (!target_bundle_dir) {
    InstallDone("getting target widevine dir failed");
    return;
  }

  Unzip(tmp_bundle_zip_file_path, *target_bundle_dir);
}

void BraveWidevineBundleManager::Unzip(
    const base::FilePath& tmp_bundle_zip_file_path,
    const base::FilePath& target_bundle_dir) {
  if (is_test_) return;

  BraveWidevineBundleUnzipper::Create(
      file_task_runner(),
      base::BindOnce(&BraveWidevineBundleManager::OnBundleUnzipped,
                     weak_factory_.GetWeakPtr()))
          ->LoadFromZipFileInDir(tmp_bundle_zip_file_path,
                                 target_bundle_dir,
                                 true);
}

void BraveWidevineBundleManager::OnBundleUnzipped(const std::string& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << __func__;
  InstallDone(error);
}

bool BraveWidevineBundleManager::in_progress() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return in_progress_;
}

void BraveWidevineBundleManager::set_in_progress(bool in_progress) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(in_progress_, in_progress);
  DVLOG(1) << __func__ << ": " << in_progress;

  in_progress_ = in_progress;
}

bool BraveWidevineBundleManager::needs_restart() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return needs_restart_;
}

void BraveWidevineBundleManager::set_needs_restart(bool needs_restart) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(needs_restart_, needs_restart);
  DVLOG(1) << __func__ << ": " << needs_restart;

  needs_restart_ = needs_restart;
}

void BraveWidevineBundleManager::InstallDone(const std::string& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  set_in_progress(false);

  // On success, marks that browser needs to restart to enable widevine.
  // On failure, don't change current prefs values.
  // If this failure comes from first install, prefs states are initial state.
  // If this failure comes from from update, prefs states are currently
  // installed state.
  if (error.empty())
    set_needs_restart(true);

  std::move(done_callback_).Run(error);
}

void BraveWidevineBundleManager::StartupCheck() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  startup_checked_ = true;

  const std::vector<content::CdmInfo>& cdms =
      content::CdmRegistry::GetInstance()->GetAllRegisteredCdms();

  auto has_widevine = [](const content::CdmInfo& cdm) {
      return cdm.supported_key_system == kWidevineKeySystem;
  };

  // If cdms has widevine cdminfo, it means that filesystem has widevine lib.
  if (std::find_if(cdms.begin(), cdms.end(), has_widevine) == cdms.end()) {
    DVLOG(1) << __func__ << ": reset widevine prefs state";
    // Widevine is not installed yet. Don't need to check.
    // Also reset prefs to make as initial state.
    ResetWidevinePrefs();
    return;
  }

  // Although this case would be very rare, this might be happen because
  // bundle unzipping and prefs setting is done asynchronously.
  if (!IsWidevineOptedIn()) {
    DVLOG(1) << __func__ << ": recover invalid widevine prefs state";
    SetWidevinePrefsAsInstalledState();
    return;
  }

  const std::string installed_version = GetWidevineInstalledVersion();

  DVLOG(1) << __func__ << ": widevine prefs state looks fine";
  DVLOG(1) << __func__ << ": installed widevine version: " << installed_version;

  // Do delayed update if installed version and latest version are different.
  if (installed_version != WIDEVINE_CDM_VERSION_STRING) {
    DVLOG(1) << __func__ << ": new widevine version("
                         << WIDEVINE_CDM_VERSION_STRING << ") is found and"
                         << " background update is scheduled.";
    update_requested_ = true;
    ScheduleBackgroundUpdate();

    return;
  }

  DVLOG(1) << __func__ << ": latest widevine version is installed.";
}

void BraveWidevineBundleManager::DeleteDeprecatedWidevineCdmLib() {
  if (is_test_) return;

  file_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          []() {
            base::FilePath deprecated_widevine_cdm_lib;
            if (base::PathService::Get(chrome::DIR_USER_DATA,
                                       &deprecated_widevine_cdm_lib)) {
              base::DeleteFile(
                  deprecated_widevine_cdm_lib
                      .Append(kWidevineCdmBaseDirectory)
                      .Append(base::GetNativeLibraryName(
                          kWidevineCdmLibraryName)));
            }
          }));
}

void BraveWidevineBundleManager::ScheduleBackgroundUpdate() {
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveWidevineBundleManager::DoDelayedBackgroundUpdate,
                     weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMinutes(GetBackgroundUpdateDelayInMins()));
}

void BraveWidevineBundleManager::OnBackgroundUpdateFinished(
    const std::string& error) {
  if (!error.empty()) {
    LOG(ERROR) << __func__ << ": " << error;
    if (background_update_retry_ < kMaxBackgroundUpdateRetry) {
      background_update_retry_++;
      DVLOG(1) << __func__ << ": " << "schedule background update again("
                                   << background_update_retry_ << ")";
      ScheduleBackgroundUpdate();
    }
    return;
  }

  DVLOG(1) << __func__ << ": Widevine update success";
  // Set new widevine version to installed version prefs.
  SetWidevinePrefsAsInstalledState();
}

void BraveWidevineBundleManager::DoDelayedBackgroundUpdate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string installed_version = GetWidevineInstalledVersion();

  DVLOG(1) << __func__ << ": update widevine"
           << " from " << installed_version
           << " to " << WIDEVINE_CDM_VERSION_STRING;

  InstallWidevineBundle(
      base::BindOnce(&BraveWidevineBundleManager::OnBackgroundUpdateFinished,
                     weak_factory_.GetWeakPtr()),
      false);
}

int
BraveWidevineBundleManager::GetWidevinePermissionRequestTextFragment() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return needs_restart() ?
      IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_RESTART_BROWSER :
      IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_INSTALL;
}

void BraveWidevineBundleManager::WillRestart() const {
  DCHECK(needs_restart());
  SetWidevinePrefsAsInstalledState();
  DVLOG(1) << __func__;
}

scoped_refptr<base::SequencedTaskRunner>
BraveWidevineBundleManager::file_task_runner() {
  if (!file_task_runner_) {
    file_task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return file_task_runner_;
}
