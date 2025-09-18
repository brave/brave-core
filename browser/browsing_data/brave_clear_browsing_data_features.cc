/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data_features.h"

namespace browsing_data::features {

BASE_FEATURE(kClearServiceWorkerCacheStorage,
             "ClearServiceWorkerCacheStorage",
             base::FEATURE_ENABLED_BY_DEFAULT);

}
