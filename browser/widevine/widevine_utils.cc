/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_utils.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace {

void ClearWidevinePrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->ClearPref(kWidevineOptedIn);
}

}  // namespace

void EnableWidevineCdmComponent(content::WebContents* web_contents) {
  DCHECK(web_contents);

  if (IsWidevineOptedIn())
    return;

  SetWidevineOptedIn(true);
  RegisterWidevineCdmComponent(g_brave_browser_process->component_updater());
}

int GetWidevinePermissionRequestTextFrangmentResourceId(bool for_restart) {
#if defined(OS_LINUX)
  return for_restart
      ? IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_RESTART_BROWSER
      : IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_INSTALL;
#else
  return IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT;
#endif
}

void RequestWidevinePermission(content::WebContents* web_contents,
                               bool for_restart) {
  permissions::PermissionRequestManager::FromWebContents(web_contents)
      ->AddRequest(web_contents->GetMainFrame(),
                   new WidevinePermissionRequest(web_contents, for_restart));
}

void DontAskWidevineInstall(content::WebContents* web_contents, bool dont_ask) {
  Profile* profile = static_cast<Profile*>(web_contents->GetBrowserContext());
  profile->GetPrefs()->SetBoolean(kAskWidevineInstall, !dont_ask);
}

void RegisterWidevineProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kWidevineOptedIn, false);
}

void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kWidevineOptedIn, false);
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
  }

  // Clear deprecated prefs.
  ClearWidevinePrefs(profile);
}
