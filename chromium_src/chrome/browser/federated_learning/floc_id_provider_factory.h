/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FEDERATED_LEARNING_FLOC_ID_PROVIDER_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FEDERATED_LEARNING_FLOC_ID_PROVIDER_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

#define BuildServiceInstanceFor                                          \
  BuildServiceInstanceFor_ChromiumImpl(content::BrowserContext* context) \
      const;                                                             \
  KeyedService* BuildServiceInstanceFor
#include "../../../../../chrome/browser/federated_learning/floc_id_provider_factory.h"
#undef BuildServiceInstanceFor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FEDERATED_LEARNING_FLOC_ID_PROVIDER_FACTORY_H_
