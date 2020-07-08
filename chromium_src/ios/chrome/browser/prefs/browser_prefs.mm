// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/prefs/browser_prefs.h"

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/sync/base/sync_prefs.h"
#include "ios/chrome/browser/pref_names.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void RegisterBrowserStatePrefs(user_prefs::PrefRegistrySyncable* registry) {
  brave_sync::Prefs::RegisterProfilePrefs(registry);
  syncer::SyncPrefs::RegisterProfilePrefs(registry);
  registry->RegisterBooleanPref(prefs::kSavingBrowserHistoryDisabled, true);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  signin::IdentityManager::RegisterLocalStatePrefs(registry);
}
