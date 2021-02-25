/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_utils.h"

#include "base/command_line.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/features.h"
#include "ui/base/l10n/l10n_util.h"

namespace permissions {

namespace {

// Returns manually set option to ease manual testing.
base::Optional<PermissionLifetimeOption> GetTestSecondsOption() {
  const char kPermissionLifetimeTestSeconds[] =
      "permission-lifetime-test-seconds";
  static base::Optional<int> test_seconds;
  if (!test_seconds.has_value()) {
    const std::string& test_seconds_str =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            kPermissionLifetimeTestSeconds);
    int val = 0;
    test_seconds = base::StringToInt(test_seconds_str, &val) ? val : 0;
  }
  if (!*test_seconds) {
    return base::nullopt;
  }
  return PermissionLifetimeOption(
      base::UTF8ToUTF16(base::StringPrintf("%d seconds", *test_seconds)),
      base::TimeDelta::FromSeconds(*test_seconds));
}

}  // namespace

std::vector<PermissionLifetimeOption> CreatePermissionLifetimeOptions() {
  std::vector<PermissionLifetimeOption> options;
  const size_t kOptionsCount = 3;
  options.reserve(kOptionsCount);

  // TODO(https://github.com/brave/brave-browser/issues/14126): Add support for
  // "until page is closed" lifetime.
#if 0
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(
          IDS_PERMISSIONS_BUBBLE_UNTIL_PAGE_CLOSE_LIFETIME_OPTION),
      base::TimeDelta()));
#endif  // 0
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(
          IDS_PERMISSIONS_BUBBLE_24_HOURS_LIFETIME_OPTION),
      base::TimeDelta::FromHours(24)));
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_1_WEEK_LIFETIME_OPTION),
      base::TimeDelta::FromDays(7)));
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_FOREVER_LIFETIME_OPTION),
      base::nullopt));
  DCHECK_EQ(options.size(), kOptionsCount);

  // This is strictly for manual testing.
  if (auto test_seconds_option = GetTestSecondsOption()) {
    options.push_back(std::move(*test_seconds_option));
  }

  return options;
}

}  // namespace permissions
