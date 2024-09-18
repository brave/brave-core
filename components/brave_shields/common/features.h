/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_FEATURES_H_

#include "base/feature_list.h"

namespace brave_shields {
namespace features {

BASE_DECLARE_FEATURE(kBraveShields);

bool IsBraveShieldsEnabled();

}  // namespace features
}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_FEATURES_H_
