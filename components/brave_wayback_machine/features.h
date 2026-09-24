// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_FEATURES_H_

#include "base/feature_list.h"

namespace brave_wayback_machine::features {

// Automatically shows the Wayback Machine bubble when the current page is
// missing (for example, a 404).
BASE_DECLARE_FEATURE(kWaybackMachineAutoShowBubble);

}  // namespace brave_wayback_machine::features

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_FEATURES_H_
