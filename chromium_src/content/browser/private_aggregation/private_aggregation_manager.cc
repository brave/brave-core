/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/private_aggregation/private_aggregation_manager.h"

#define GetManager GetManager_ChromiumImpl
#include "src/content/browser/private_aggregation/private_aggregation_manager.cc"
#undef GetManager

namespace content {

PrivateAggregationManager* PrivateAggregationManager::GetManager(
    BrowserContext& browser_context) {
  return nullptr;
}

}  // namespace content
