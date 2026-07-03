// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_REPORTER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PSST_PSST_REPORTER_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

class Profile;

namespace psst {
class PsstReporterService;
}  // namespace psst

class PsstReporterServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static PsstReporterServiceFactory* GetInstance();
  static psst::PsstReporterService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<PsstReporterServiceFactory>;

  PsstReporterServiceFactory();
  ~PsstReporterServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_PSST_PSST_REPORTER_SERVICE_FACTORY_H_
