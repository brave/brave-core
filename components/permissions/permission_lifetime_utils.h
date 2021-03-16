/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_UTILS_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_UTILS_H_

#include <vector>

#include "base/optional.h"
#include "base/time/time.h"
#include "brave/components/permissions/permission_lifetime_options.h"

namespace permissions {

// Returns pre-configured permission lifetime options.
std::vector<PermissionLifetimeOption> CreatePermissionLifetimeOptions();

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_UTILS_H_
