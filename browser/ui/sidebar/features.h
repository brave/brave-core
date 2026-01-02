// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SIDEBAR_FEATURES_H_
#define BRAVE_BROWSER_UI_SIDEBAR_FEATURES_H_

#include "base/feature_list.h"

namespace sidebar::features {

// Feature flag for the effort that separate side panel from
// SidebarContainerView.
// See https://github.com/brave/brave-browser/issues/51462 for more details.
BASE_DECLARE_FEATURE(kSidebarV2);

}  // namespace sidebar::features

#endif  // BRAVE_BROWSER_UI_SIDEBAR_FEATURES_H_
