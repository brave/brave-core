/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_referrals/referrals_service_delegate.h"

#include "base/path_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/chrome_paths.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/first_run/first_run.h"
#endif  // !BUILDFLAG(!IS_ANDROID)

ReferralsServiceDelegate::ReferralsServiceDelegate(
    brave::BraveReferralsService* service)
    : service_(service) {
  if (auto* profile_manager = g_browser_process->profile_manager()) {
    profile_manager_observation_.Observe(profile_manager);
    DCHECK_EQ(0U, profile_manager->GetLoadedProfiles().size());
  }
}
ReferralsServiceDelegate::~ReferralsServiceDelegate() = default;

void ReferralsServiceDelegate::OnInitialized() {
  profile_manager_observation_.Reset();
}

base::FilePath ReferralsServiceDelegate::GetUserDataDirectory() {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return user_data_dir;
}

network::mojom::URLLoaderFactory*
ReferralsServiceDelegate::GetURLLoaderFactory() {
  return g_browser_process->system_network_context_manager()
      ->GetURLLoaderFactory();
}

#if !BUILDFLAG(IS_ANDROID)
base::OnceCallback<base::Time()>
ReferralsServiceDelegate::GetFirstRunSentinelCreationTimeCallback() {
  return base::BindOnce(first_run::GetFirstRunSentinelCreationTime);
}
#endif

void ReferralsServiceDelegate::OnProfileAdded(Profile* profile) {
  if (profile != ProfileManager::GetLastUsedProfileIfLoaded())
    return;

  service_->Start();
  DCHECK(!profile_manager_observation_.IsObserving())
      << "Should be cleared by OnInitialized";
}
