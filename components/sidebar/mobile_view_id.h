/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_MOBILE_VIEW_ID_H_
#define BRAVE_COMPONENTS_SIDEBAR_MOBILE_VIEW_ID_H_

#include <string>

#include "base/types/strong_alias.h"

namespace sidebar {

using MobileViewId = base::StrongAlias<class MobileViewIdTag, std::string>;

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_MOBILE_VIEW_ID_H_
