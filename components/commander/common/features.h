// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_COMMANDER_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace features {

COMPONENT_EXPORT(COMMANDER_COMMON) BASE_DECLARE_FEATURE(kBraveCommander);

}

namespace commander {

COMPONENT_EXPORT(COMMANDER_COMMON) bool CommanderEnabled();

}  // namespace commander

#endif  // BRAVE_COMPONENTS_COMMANDER_COMMON_FEATURES_H_
