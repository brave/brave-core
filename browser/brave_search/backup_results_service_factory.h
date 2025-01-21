// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_search {
class BackupResultsService;

class BackupResultsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  BackupResultsServiceFactory(const BackupResultsServiceFactory&) = delete;
  BackupResultsServiceFactory& operator=(const BackupResultsServiceFactory&) =
      delete;

  static BackupResultsServiceFactory* GetInstance();
  static BackupResultsService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<BackupResultsServiceFactory>;

  BackupResultsServiceFactory();
  ~BackupResultsServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace brave_search

#endif  // BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_FACTORY_H_
