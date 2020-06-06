/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/pref_service_builder_utils.h"

#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/profiles/brave_profile_impl.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/pref_service_syncable_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/spellcheck/browser/pref_names.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#endif

#define CreatePrefService CreatePrefService_ChromiumImpl
#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl
#include "../../../../../chrome/browser/profiles/pref_service_builder_utils.cc"
#undef CreatePrefService
#undef RegisterProfilePrefs

// Prefs for KeyedService
void RegisterProfilePrefs(bool is_signin_profile,
                          const std::string& locale,
                          user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs_ChromiumImpl(is_signin_profile, locale, registry);

  brave_rewards::RewardsService::RegisterProfilePrefs(registry);

  // Disable spell check service
  registry->SetDefaultPrefValue(
      spellcheck::prefs::kSpellCheckUseSpellingService, base::Value(false));

  // Make sure sign into Brave is not enabled
  // The older kSigninAllowed is deprecated and only in use in Android until
  // C71.
  registry->SetDefaultPrefValue(prefs::kSigninAllowedOnNextStartup,
                                base::Value(false));

#if defined(OS_LINUX)
  // Use brave theme by default instead of gtk theme.
  registry->SetDefaultPrefValue(prefs::kUsesSystemTheme, base::Value(false));
#endif
}

std::unique_ptr<sync_preferences::PrefServiceSyncable> CreatePrefService(
    scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry,
    PrefStore* extension_pref_store,
    policy::PolicyService* policy_service,
    policy::ChromeBrowserPolicyConnector* browser_policy_connector,
    mojo::PendingRemote<prefs::mojom::TrackedPreferenceValidationDelegate>
        pref_validation_delegate,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    SimpleFactoryKey* key,
    const base::FilePath& path,
    bool async_prefs) {
  // Create prefs using the same approach that chromium used when creating an
  // off-the-record profile from its original profile.
  if (brave::IsSessionProfilePath(path)) {
    base::FilePath original_path = brave::GetParentProfilePath(path);
    Profile* original_profile =
        g_browser_process->profile_manager()->GetProfileByPath(original_path);
    DCHECK(original_profile);
    PrefStore* extension_pref_store = nullptr;
#if BUILDFLAG(ENABLE_EXTENSIONS)
    extension_pref_store = new ExtensionPrefStore(
        ExtensionPrefValueMapFactory::GetForBrowserContext(original_profile),
        true);
#endif
    return CreateIncognitoPrefServiceSyncable(
        PrefServiceSyncableFromProfile(original_profile), extension_pref_store);
  }

  return CreatePrefService_ChromiumImpl(
      pref_registry, extension_pref_store, policy_service,
      browser_policy_connector, std::move(pref_validation_delegate),
      io_task_runner, key, path, async_prefs);
}
