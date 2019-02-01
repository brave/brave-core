/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_widevine_bundle_util.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/browser/media/cdm_registry_impl.h"
#include "content/public/browser/browser_thread.h"

void MaybeRegisterWidevineCdm() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  // Only registers widevine cdm when user explicitly requested.
  if (!prefs->GetBoolean(kWidevineOptedIn))
    return;

  auto* cdm_registry = content::CdmRegistryImpl::GetInstance();
  cdm_registry->RegisterCdm(cdm_registry->cached_widevine_cdm_info());
}
