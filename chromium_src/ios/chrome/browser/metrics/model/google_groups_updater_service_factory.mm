/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/metrics/model/google_groups_updater_service_factory.h"

#define BuildServiceInstanceFor BuildServiceInstanceFor_ChromiumImpl
#include "src/ios/chrome/browser/metrics/model/google_groups_updater_service_factory.mm"
#undef BuildServiceInstanceFor

std::unique_ptr<KeyedService>
GoogleGroupsUpdaterServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return nullptr;
}
