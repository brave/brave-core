// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_SETTINGS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PSST_PSST_SETTINGS_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

class Profile;

namespace psst {
class PsstSettingsService;
}  // namespace psst

// Profile keyed service factory for the `PsstSettingsService`.
class PsstSettingsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the singleton instance of `PsstSettingsServiceFactory`.
  static PsstSettingsServiceFactory* GetInstance();

  // Returns the instance of `PsstSettingsServiceFactory` for the passed
  // `profile`.
  static psst::PsstSettingsService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<PsstSettingsServiceFactory>;

  PsstSettingsServiceFactory();
  ~PsstSettingsServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  //
  // Returns nullptr when PSST is disabled via feature flag, or explicitly
  // disabled via policy.
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_PSST_PSST_SETTINGS_SERVICE_FACTORY_H_
