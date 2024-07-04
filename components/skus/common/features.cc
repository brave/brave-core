/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/common/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace skus::features {

BASE_FEATURE(kSkusFeature, "SkusFeature", base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace skus::features
