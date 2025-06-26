/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/optimization_guide/core/optimization_guide_permissions_util.h"

#define IsUserPermittedToFetchFromRemoteOptimizationGuide \
  IsUserPermittedToFetchFromRemoteOptimizationGuide_ChromiumImpl
#include "src/components/optimization_guide/core/optimization_guide_permissions_util.cc"
#undef IsUserPermittedToFetchFromRemoteOptimizationGuide

namespace optimization_guide {

bool IsUserPermittedToFetchFromRemoteOptimizationGuide(
    bool is_off_the_record,
    PrefService* pref_service) {
  return false;
}

}  // namespace optimization_guide
