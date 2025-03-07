/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/prediction_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_selections.h"

// static
permissions::PredictionService* PredictionServiceFactory::GetForProfile(
    Profile* profile) {
  return nullptr;
}

// static
PredictionServiceFactory* PredictionServiceFactory::GetInstance() {
  static base::NoDestructor<PredictionServiceFactory> instance;
  return instance.get();
}

PredictionServiceFactory::PredictionServiceFactory()
    : ProfileKeyedServiceFactory("PredictionService",
                                 ProfileSelections::BuildNoProfilesSelected()) {
}

PredictionServiceFactory::~PredictionServiceFactory() = default;

std::unique_ptr<KeyedService>
PredictionServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}
