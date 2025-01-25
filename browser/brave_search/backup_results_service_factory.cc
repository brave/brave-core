// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/brave_search/backup_results_service_impl.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_search {

// static
BackupResultsServiceFactory* BackupResultsServiceFactory::GetInstance() {
  static base::NoDestructor<BackupResultsServiceFactory> instance;
  return instance.get();
}

// static
BackupResultsService* BackupResultsServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BackupResultsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BackupResultsServiceFactory::BackupResultsServiceFactory()
    : ProfileKeyedServiceFactory(
          "BackupResultsService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              .WithGuest(ProfileSelection::kRedirectedToOriginal)
              .Build()) {}

BackupResultsServiceFactory::~BackupResultsServiceFactory() = default;

std::unique_ptr<KeyedService>
BackupResultsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BackupResultsServiceImpl>(
      Profile::FromBrowserContext(context));
}

}  // namespace brave_search
