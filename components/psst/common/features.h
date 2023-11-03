// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_PSST_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace psst::features {

COMPONENT_EXPORT(PSST_COMMON) BASE_DECLARE_FEATURE(kBravePsst);

}  // namespace psst::features

#endif  // BRAVE_COMPONENTS_PSST_COMMON_FEATURES_H_
