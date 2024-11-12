/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_options.h"

#include <optional>
#include <utility>

namespace permissions {

PermissionLifetimeOption::PermissionLifetimeOption(
    std::u16string label,
    std::optional<base::TimeDelta> lifetime)
    : label(std::move(label)), lifetime(lifetime) {}

PermissionLifetimeOption::PermissionLifetimeOption(
    const PermissionLifetimeOption&) = default;
PermissionLifetimeOption& PermissionLifetimeOption::operator=(
    const PermissionLifetimeOption&) = default;
PermissionLifetimeOption::PermissionLifetimeOption(
    PermissionLifetimeOption&&) noexcept = default;
PermissionLifetimeOption& PermissionLifetimeOption::operator=(
    PermissionLifetimeOption&&) noexcept = default;
PermissionLifetimeOption::~PermissionLifetimeOption() = default;

}  // namespace permissions
