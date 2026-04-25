// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/containers/containers_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/containers/containers_service_delegate.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/prefs_registration.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/common/buildflags.h"

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/session_service_factory.h"
#endif

// static
ContainersServiceFactory* ContainersServiceFactory::GetInstance() {
  static base::NoDestructor<ContainersServiceFactory> instance;
  return instance.get();
}

// static
containers::ContainersService* ContainersServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<containers::ContainersService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

ContainersServiceFactory::ContainersServiceFactory()
    : ProfileKeyedServiceFactory(
          "ContainersService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .Build()) {
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  DependsOn(SessionServiceFactory::GetInstance());
#endif
  DependsOn(TabRestoreServiceFactory::GetInstance());
}

ContainersServiceFactory::~ContainersServiceFactory() = default;

void ContainersServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  containers::RegisterProfilePrefs(registry);
}

bool ContainersServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

std::unique_ptr<KeyedService>
ContainersServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return nullptr;
  }

  auto* profile = Profile::FromBrowserContext(context);
  return std::make_unique<containers::ContainersService>(
      profile->GetPrefs(),
      std::make_unique<ContainersServiceDelegate>(
          profile,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
          SessionServiceFactory::GetForProfile(profile),
#endif
          TabRestoreServiceFactory::GetForProfile(profile)));
}
