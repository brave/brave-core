// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_log_service_factory.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/psst/core/browser/psst_log_service.h"
#include "brave/components/psst/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "content/public/browser/browser_context.h"

// static
PsstLogServiceFactory* PsstLogServiceFactory::GetInstance() {
  static base::NoDestructor<PsstLogServiceFactory> instance;
  return instance.get();
}

// static
psst::PsstLogService* PsstLogServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<psst::PsstLogService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
psst::PsstLogService* PsstLogServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  auto* profile = Profile::FromBrowserContext(context);
  return profile ? GetForProfile(profile) : nullptr;
}

PsstLogServiceFactory::PsstLogServiceFactory()
    : ProfileKeyedServiceFactory(
          "PsstLogService",
          // OTR profiles get no service, so PSST logs are never collected in
          // Incognito or Tor windows.
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

PsstLogServiceFactory::~PsstLogServiceFactory() = default;

std::unique_ptr<KeyedService>
PsstLogServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    return nullptr;
  }
  return std::make_unique<psst::PsstLogService>();
}
