/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_METRICS_MODEL_GOOGLE_GROUPS_UPDATER_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_METRICS_MODEL_GOOGLE_GROUPS_UPDATER_SERVICE_FACTORY_H_

// Include to prevent redefining BuildServiceInstanceFor
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

#define BuildServiceInstanceFor                                           \
  BuildServiceInstanceFor_ChromiumImpl(web::BrowserState* context) const; \
  std::unique_ptr<KeyedService> BuildServiceInstanceFor

#include "src/ios/chrome/browser/metrics/model/google_groups_updater_service_factory.h"  // IWYU pragma: export
#undef BuildServiceInstanceFor

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_METRICS_MODEL_GOOGLE_GROUPS_UPDATER_SERVICE_FACTORY_H_
