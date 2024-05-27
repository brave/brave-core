/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_utils.h"

#include <string>

#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/widevine/constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/permissions/permission_request_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/widevine/cdm/buildflags.h"

namespace {

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
void InstallWidevineOnceRegistered() {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()->OnDemandInstall(
      kWidevineComponentId);
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

void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kWidevineEnabled, false);
}

bool IsWidevineEnabled() {
  // N.B.: As of this writing, kWidevineEnabled is also queried in other places.
  // If you want to change the logic for enabling Widevine, then you need to
  // change those other places as well.
  return g_browser_process->local_state()->GetBoolean(kWidevineEnabled);
}

void SetWidevineEnabled(bool opted_in) {
  g_browser_process->local_state()->SetBoolean(kWidevineEnabled, opted_in);
}
