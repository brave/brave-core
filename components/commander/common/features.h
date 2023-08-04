// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_COMMANDER_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace features {

// Note: This flag is declared in features rather than commander::features so we
// can replace the upstream flag with it more easily.
COMPONENT_EXPORT(COMMANDER_COMMON) BASE_DECLARE_FEATURE(kBraveCommander);

}  // namespace features

#endif  // BRAVE_COMPONENTS_COMMANDER_COMMON_FEATURES_H_
