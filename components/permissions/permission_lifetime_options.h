/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_OPTIONS_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_OPTIONS_H_

#include <vector>

#include "base/optional.h"
#include "base/time/time.h"

namespace permissions {

struct PermissionLifetimeOption {
  PermissionLifetimeOption(std::u16string label,
                           base::Optional<base::TimeDelta> lifetime);
  PermissionLifetimeOption(const PermissionLifetimeOption&);
  PermissionLifetimeOption& operator=(const PermissionLifetimeOption&);
  PermissionLifetimeOption(PermissionLifetimeOption&&) noexcept;
  PermissionLifetimeOption& operator=(PermissionLifetimeOption&&) noexcept;
  ~PermissionLifetimeOption();

  // Text visible to the user.
  std::u16string label;
  // If not set, lifetime will not be controlled (i.e. permanent). If set to
  // base::TimeDelta(), permission should be alive until eTLD+1 is closed.
  base::Optional<base::TimeDelta> lifetime;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_OPTIONS_H_
