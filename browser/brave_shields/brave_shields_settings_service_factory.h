// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

class Profile;

namespace brave_shields {
class BraveShieldsSettingsService;
}

class BraveShieldsSettingsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BraveShieldsSettingsServiceFactory* GetInstance();
  static brave_shields::BraveShieldsSettingsService* GetForProfile(
      Profile* profile);

 private:
  friend base::NoDestructor<BraveShieldsSettingsServiceFactory>;

  BraveShieldsSettingsServiceFactory();
  ~BraveShieldsSettingsServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_H_
