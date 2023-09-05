/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_utils.h"

#include <string>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/widevine/constants.h"
#include "brave/components/widevine/static_buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/component_updater_utils.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/permissions/permission_request_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

using content::BrowserThread;

namespace {

#if BUILDFLAG(IS_LINUX)
constexpr char kWidevineInvalidVersion[] = "";

// Added 11/2020.
constexpr char kWidevineInstalledVersion[] = "brave.widevine_installed_version";

void OnDeletedOldWidevineBinary(bool result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (result) {
    auto* local_state = g_browser_process->local_state();
    local_state->ClearPref(kWidevineInstalledVersion);
  }
}

bool DoDeleteOldWidevineBinary() {
  base::FilePath widevine_base_path;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &widevine_base_path))
    return false;

  widevine_base_path =
      widevine_base_path.AppendASCII(kWidevineCdmBaseDirectory);
  const base::FilePath manifest_file_path =
      widevine_base_path.AppendASCII("manifest.json");
  const base::FilePath platform_specific_dir_path =
      widevine_base_path.AppendASCII("_platform_specific");
  return base::DeleteFile(manifest_file_path) &&
         base::DeletePathRecursively(platform_specific_dir_path);
}

void DeleteOldWidevineBinary() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&DoDeleteOldWidevineBinary),
      base::BindOnce(&OnDeletedOldWidevineBinary));
}
#endif

void ClearWidevinePrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->ClearPref(kWidevineEnabled);
}

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
void InstallWidevineOnceRegistered() {
  component_updater::BraveOnDemandUpdate(kWidevineComponentId);
}
#endif
}  // namespace

void EnableWidevineCdm() {
  if (IsWidevineEnabled()) {
    return;
  }

  SetWidevineEnabled(true);
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  RegisterWidevineCdmComponent(g_browser_process->component_updater(),
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
                               g_browser_process->shared_url_loader_factory(),
#endif
                               base::BindOnce(&InstallWidevineOnceRegistered));
#endif
}

void DisableWidevineCdm() {
  if (!IsWidevineEnabled()) {
    return;
  }

  SetWidevineEnabled(false);
}

int GetWidevinePermissionRequestTextFrangmentResourceId(bool for_restart) {
#if BUILDFLAG(IS_LINUX)
  return for_restart
             ? IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_RESTART_BROWSER
             : IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_INSTALL;
#elif BUILDFLAG(IS_ANDROID)
  return IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_ANDROID;
#else
  return IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT;
#endif
}

void RequestWidevinePermission(content::WebContents* web_contents,
                               bool for_restart) {
  permissions::PermissionRequestManager::FromWebContents(web_contents)
      ->AddRequest(web_contents->GetPrimaryMainFrame(),
                   new WidevinePermissionRequest(web_contents, for_restart));
}

void RegisterWidevineProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kWidevineEnabled, false);
}

void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kWidevineEnabled, false);
}

bool IsWidevineEnabled() {
  return g_browser_process->local_state()->GetBoolean(kWidevineEnabled);
}

void SetWidevineEnabled(bool opted_in) {
  g_browser_process->local_state()->SetBoolean(kWidevineEnabled, opted_in);
}

void MigrateWidevinePrefs(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* local_state = g_browser_process->local_state();
  // If migration is done, local state doesn't have default value because
  // they were explicitly set by primary prefs' value. After that, we don't
  // need to try migration again and prefs from profiles are already cleared.
  if (local_state->FindPreference(kWidevineEnabled)->IsDefaultValue()) {
    PrefService* prefs = profile->GetPrefs();
    local_state->SetBoolean(kWidevineEnabled,
                            prefs->GetBoolean(kWidevineEnabled));
  }

  // Clear deprecated prefs.
  ClearWidevinePrefs(profile);
}

void RegisterWidevineLocalstatePrefsForMigration(PrefRegistrySimple* registry) {
#if BUILDFLAG(IS_LINUX)
  registry->RegisterStringPref(kWidevineInstalledVersion,
                               kWidevineInvalidVersion);
#endif
}

void MigrateObsoleteWidevineLocalStatePrefs(PrefService* local_state) {
#if BUILDFLAG(IS_LINUX)
  // If local state doesn't have default value, it means we've used old
  // widevine binary. Delete old widevine binary.
  if (!local_state->FindPreference(kWidevineInstalledVersion)
           ->IsDefaultValue()) {
    DeleteOldWidevineBinary();
  }
#endif
}
