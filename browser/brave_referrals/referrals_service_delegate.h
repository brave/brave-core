/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REFERRALS_REFERRALS_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_REFERRALS_REFERRALS_SERVICE_DELEGATE_H_

#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_manager_observer.h"

class ReferralsServiceDelegate : public brave::BraveReferralsService::Delegate,
                                 public ProfileManagerObserver {
 public:
  explicit ReferralsServiceDelegate(brave::BraveReferralsService* service);
  ~ReferralsServiceDelegate() override;

  // brave::BraveReferralsService::Delegate:
  void OnInitialized() override;
  base::FilePath GetUserDataDirectory() override;
  network::mojom::URLLoaderFactory* GetURLLoaderFactory() override;
#if !BUILDFLAG(IS_ANDROID)
  base::OnceCallback<base::Time()> GetFirstRunSentinelCreationTimeCallback()
      override;
#endif

  // ProfileManagerObserver:
  void OnProfileAdded(Profile* profile) override;

 private:
  raw_ptr<brave::BraveReferralsService> service_;  // owner

  base::ScopedObservation<ProfileManager, ProfileManagerObserver>
      profile_manager_observation_{this};
};

#endif  // BRAVE_BROWSER_BRAVE_REFERRALS_REFERRALS_SERVICE_DELEGATE_H_
