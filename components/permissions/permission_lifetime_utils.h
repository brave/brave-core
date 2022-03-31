/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_UTILS_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_UTILS_H_

#include <vector>

#include "base/time/time.h"
#include "brave/components/permissions/permission_lifetime_options.h"
#include "components/permissions/permission_prompt.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace permissions {

// Returns pre-configured permission lifetime options.
std::vector<PermissionLifetimeOption> CreatePermissionLifetimeOptions();

// Returns true if all queued requests support the lifetime setting.
bool ShouldShowLifetimeOptions(PermissionPrompt::Delegate* delegate);

// Sets selected lifetime in all queued requests.
void SetRequestsLifetime(const std::vector<PermissionLifetimeOption>& options,
                         int index,
                         PermissionPrompt::Delegate* delegate);

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_UTILS_H_
