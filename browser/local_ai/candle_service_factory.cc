// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/candle_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/local_ai/content/candle_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"

namespace local_ai {

// static
CandleServiceFactory* CandleServiceFactory::GetInstance() {
  static base::NoDestructor<CandleServiceFactory> instance;
  return instance.get();
}

// static
CandleService* CandleServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<CandleService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

CandleServiceFactory::CandleServiceFactory()
    : ProfileKeyedServiceFactory(
          "CandleService",
          ProfileSelections::BuildRedirectedInIncognito()) {}

CandleServiceFactory::~CandleServiceFactory() = default;

std::unique_ptr<KeyedService>
CandleServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<CandleService>(context);
}

}  // namespace local_ai
