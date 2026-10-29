// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_LOG_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PSST_PSST_LOG_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

class Profile;

namespace psst {
class PsstLogService;
}  // namespace psst

// Creates the PSST diagnostics log service. Returns null — and so disables the
// whole logging feature, including brave://psst-internals — for off-the-record
// (Incognito, Tor, Guest) profiles and when the PSST feature flag is off.
// Callers treat a null service as "logging unavailable".
class PsstLogServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static PsstLogServiceFactory* GetInstance();
  static psst::PsstLogService* GetForProfile(Profile* profile);
  static psst::PsstLogService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<PsstLogServiceFactory>;

  PsstLogServiceFactory();
  ~PsstLogServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_PSST_PSST_LOG_SERVICE_FACTORY_H_
