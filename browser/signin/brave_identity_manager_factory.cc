/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/signin/brave_identity_manager_factory.h"

#include <memory>

#include "base/files/file_path.h"
#include "brave/components/signin/public/identity_manager/brave_identity_manager.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "components/signin/public/identity_manager/identity_manager_builder.h"
#include "components/signin/public/webdata/token_web_data.h"
#include "content/public/browser/network_service_instance.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/web_data_service_factory.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/signin/core/browser/cookie_settings_util.h"
#endif

BraveIdentityManagerFactory::BraveIdentityManagerFactory()
    : IdentityManagerFactory() {}

BraveIdentityManagerFactory::~BraveIdentityManagerFactory() {}

// static
signin::BraveIdentityManager* BraveIdentityManagerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<signin::BraveIdentityManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
signin::BraveIdentityManager*
BraveIdentityManagerFactory::GetForProfileIfExists(const Profile* profile) {
  return static_cast<signin::BraveIdentityManager*>(
      GetInstance()->GetServiceForBrowserContext(const_cast<Profile*>(profile),
                                                 false));
}

// static
BraveIdentityManagerFactory* BraveIdentityManagerFactory::GetInstance() {
  return base::Singleton<BraveIdentityManagerFactory>::get();
}

KeyedService* BraveIdentityManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  signin::IdentityManagerBuildParams params;
  params.account_consistency =
      AccountConsistencyModeManager::GetMethodForProfile(profile),
  params.image_decoder = std::make_unique<ImageDecoderImpl>();
  params.local_state = g_browser_process->local_state();
  params.network_connection_tracker = content::GetNetworkConnectionTracker();
  params.pref_service = profile->GetPrefs();
  params.profile_path = profile->GetPath();
  params.signin_client = ChromeSigninClientFactory::GetForProfile(profile);

#if !defined(OS_ANDROID)
  params.delete_signin_cookies_on_exit =
      signin::SettingsDeleteSigninCookiesOnExit(
          CookieSettingsFactory::GetForProfile(profile).get());
  params.token_web_data = WebDataServiceFactory::GetTokenWebDataForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
#endif

  std::unique_ptr<signin::BraveIdentityManager> brave_identity_manager =
      signin::BuildBraveIdentityManager(&params);

  return brave_identity_manager.release();
}

void BraveIdentityManagerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}
