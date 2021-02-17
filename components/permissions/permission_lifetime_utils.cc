/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_utils.h"

#include <utility>

#include "base/i18n/time_formatting.h"
#include "base/stl_util.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace permissions {

PermissionLifetimeOption::PermissionLifetimeOption(
    base::string16 label,
    base::Optional<base::TimeDelta> lifetime)
    : label(std::move(label)), lifetime(std::move(lifetime)) {}

PermissionLifetimeOption::PermissionLifetimeOption(
    const PermissionLifetimeOption&) = default;
PermissionLifetimeOption& PermissionLifetimeOption::operator=(
    const PermissionLifetimeOption&) = default;
PermissionLifetimeOption::PermissionLifetimeOption(
    PermissionLifetimeOption&&) noexcept = default;
PermissionLifetimeOption& PermissionLifetimeOption::operator=(
    PermissionLifetimeOption&&) noexcept = default;
PermissionLifetimeOption::~PermissionLifetimeOption() = default;

std::vector<PermissionLifetimeOption> CreatePermissionLifetimeOptions() {
  // TODO(https://github.com/brave/brave-browser/issues/14126): Actualize
  // values.
  constexpr base::TimeDelta kLifetimes[] = {
      base::TimeDelta::FromSeconds(60), base::TimeDelta::FromHours(1),
      base::TimeDelta::FromHours(3),    base::TimeDelta::FromDays(1),
      base::TimeDelta::FromDays(30),
  };

  std::vector<PermissionLifetimeOption> options;
  options.reserve(1 + base::size(kLifetimes));
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(
          IDS_PERMISSIONS_BUBBLE_PERMANENT_LIFETIME_OPTION),
      base::nullopt));

  for (const auto& lifetime : kLifetimes) {
    base::string16 formatted_time;
    const bool format_successful = base::TimeDurationFormat(
        lifetime, base::DURATION_WIDTH_WIDE, &formatted_time);
    DCHECK(format_successful);
    options.emplace_back(
        PermissionLifetimeOption(std::move(formatted_time), lifetime));
  }

  return options;
}

}  // namespace permissions
