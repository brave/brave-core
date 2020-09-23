/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_utils.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include <string>

#include "base/bind.h"
#include "brave/browser/brave_drm_tab_helper.h"
#include "brave/browser/widevine/brave_widevine_bundle_manager.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#endif

using content::BrowserThread;

namespace {

void ClearWidevinePrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->ClearPref(kWidevineOptedIn);

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  prefs->ClearPref(kWidevineInstalledVersion);
#endif
}

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
content::WebContents* GetActiveWebContents() {
  if (Browser* browser = chrome::FindLastActive())
    return browser->tab_strip_model()->GetActiveWebContents();
  return nullptr;
}

bool IsActiveTabRequestedWidevine() {
  bool is_active_tab_requested_widevine = false;
  if (content::WebContents* active_web_contents = GetActiveWebContents()) {
    BraveDrmTabHelper* drm_helper =
        BraveDrmTabHelper::FromWebContents(active_web_contents);
    is_active_tab_requested_widevine = drm_helper->ShouldShowWidevineOptIn();
  }

  return is_active_tab_requested_widevine;
}

void OnWidevineInstallDone(const std::string& error) {
  if (!error.empty()) {
    LOG(ERROR) << __func__ << ": " << error;
    return;
  }

  DVLOG(1) << __func__ << ": Widevine install success";
  // Request Widevine permission bubble for restart browser.
  if (IsActiveTabRequestedWidevine())
    RequestWidevinePermission(GetActiveWebContents());
}
#endif

}  // namespace

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
void EnableWidevineCdmComponent(content::WebContents* web_contents) {
  DCHECK(web_contents);

  if (IsWidevineOptedIn())
    return;

  SetWidevineOptedIn(true);
  RegisterWidevineCdmComponent(g_brave_browser_process->component_updater());
}
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
void InstallBundleOrRestartBrowser() {
  auto* manager = g_brave_browser_process->brave_widevine_bundle_manager();
  if (manager->needs_restart()) {
    manager->WillRestart();
    if (!manager->is_test()) {
      // Prevent relaunch during the browser test.
      // This will cause abnormal termination.
      chrome::AttemptRelaunch();
    }
    return;
  }

  // User can request install again because |kWidevineOptedIn| is set when
  // install is finished. In this case, just waiting previous install request.
  if (!manager->in_progress()) {
    manager->InstallWidevineBundle(base::BindOnce(&OnWidevineInstallDone),
                                   true);
  }
}

void SetWidevineInstalledVersion(const std::string& version) {
  g_browser_process->local_state()->SetString(kWidevineInstalledVersion,
                                              version);
}

std::string GetWidevineInstalledVersion() {
  return g_browser_process->local_state()->GetString(kWidevineInstalledVersion);
}
#endif

int GetWidevinePermissionRequestTextFrangmentResourceId() {
  int message_id = -1;

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  message_id = IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT;
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  auto* manager = g_brave_browser_process->brave_widevine_bundle_manager();
  message_id = manager->GetWidevinePermissionRequestTextFragment();
#endif

  DCHECK_NE(message_id, -1);
  return message_id;
}

void RequestWidevinePermission(content::WebContents* web_contents) {
  permissions::PermissionRequestManager::FromWebContents(web_contents)
      ->AddRequest(web_contents->GetMainFrame(),
                   new WidevinePermissionRequest(web_contents));
}
void DontAskWidevineInstall(content::WebContents* web_contents, bool dont_ask) {
  Profile* profile = static_cast<Profile*>(web_contents->GetBrowserContext());
  profile->GetPrefs()->SetBoolean(kAskWidevineInstall, !dont_ask);
}

void RegisterWidevineProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kWidevineOptedIn, false);

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  registry->RegisterStringPref(
      kWidevineInstalledVersion,
      BraveWidevineBundleManager::kWidevineInvalidVersion);
#endif
}

void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kWidevineOptedIn, false);

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  registry->RegisterStringPref(
      kWidevineInstalledVersion,
      BraveWidevineBundleManager::kWidevineInvalidVersion);
#endif
}

bool IsWidevineOptedIn() {
  return g_browser_process->local_state()->GetBoolean(kWidevineOptedIn);
}

void SetWidevineOptedIn(bool opted_in) {
  g_browser_process->local_state()->SetBoolean(kWidevineOptedIn, opted_in);
}

void MigrateWidevinePrefs(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* local_state = g_browser_process->local_state();
  // If migration is done, local state doesn't have default value because
  // they were explicitly set by primary prefs' value. After that, we don't
  // need to try migration again and prefs from profiles are already cleared.
  if (local_state->FindPreference(kWidevineOptedIn)->IsDefaultValue()) {
    PrefService* prefs = profile->GetPrefs();
    local_state->SetBoolean(kWidevineOptedIn,
                            prefs->GetBoolean(kWidevineOptedIn));

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
    local_state->SetString(kWidevineInstalledVersion,
                           prefs->GetString(kWidevineInstalledVersion));
#endif
  }

  // Clear deprecated prefs.
  ClearWidevinePrefs(profile);
}
