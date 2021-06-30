/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_utils.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/features.h"
#include "components/permissions/permission_request.h"
#include "net/base/features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"

namespace permissions {

namespace {

// Returns manually set option to ease manual testing.
absl::optional<PermissionLifetimeOption> GetTestSecondsOption() {
  const char kPermissionLifetimeTestSeconds[] =
      "permission-lifetime-test-seconds";
  static absl::optional<int> test_seconds;
  if (!test_seconds.has_value()) {
    const std::string& test_seconds_str =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            kPermissionLifetimeTestSeconds);
    int val = 0;
    test_seconds = base::StringToInt(test_seconds_str, &val) ? val : 0;
  }
  if (!*test_seconds) {
    return absl::nullopt;
  }
  return PermissionLifetimeOption(
      base::UTF8ToUTF16(base::StringPrintf("%d seconds", *test_seconds)),
      base::TimeDelta::FromSeconds(*test_seconds));
}

}  // namespace

std::vector<PermissionLifetimeOption> CreatePermissionLifetimeOptions() {
  std::vector<PermissionLifetimeOption> options;
  const size_t kOptionsCount = 4;
  options.reserve(kOptionsCount);

  if (base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage)) {
    options.emplace_back(PermissionLifetimeOption(
        l10n_util::GetStringUTF16(
            IDS_PERMISSIONS_BUBBLE_UNTIL_PAGE_CLOSE_LIFETIME_OPTION),
        base::TimeDelta()));
  }
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(
          IDS_PERMISSIONS_BUBBLE_24_HOURS_LIFETIME_OPTION),
      base::TimeDelta::FromHours(24)));
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_1_WEEK_LIFETIME_OPTION),
      base::TimeDelta::FromDays(7)));
  options.emplace_back(PermissionLifetimeOption(
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_FOREVER_LIFETIME_OPTION),
      absl::nullopt));
  DCHECK_LE(options.size(), kOptionsCount);

  // This is strictly for manual testing.
  if (auto test_seconds_option = GetTestSecondsOption()) {
    options.push_back(std::move(*test_seconds_option));
  }

  return options;
}

bool ShouldShowLifetimeOptions(PermissionPrompt::Delegate* delegate) {
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return false;
  }

  const bool all_requests_support_lifetime = std::all_of(
      delegate->Requests().begin(), delegate->Requests().end(),
      [](const auto& request) { return request->SupportsLifetime(); });
  return all_requests_support_lifetime;
}

void SetRequestsLifetime(const std::vector<PermissionLifetimeOption>& options,
                         int index,
                         PermissionPrompt::Delegate* delegate) {
  DCHECK(base::FeatureList::IsEnabled(features::kPermissionLifetime));
  DCHECK(!options.empty());
  DCHECK(index >= 0 && static_cast<size_t>(index) < options.size());
  const auto& lifetime = options[index].lifetime;
  DLOG(INFO) << "Set permission lifetime "
             << (lifetime ? lifetime->InSeconds() : -1);
  for (auto* request : delegate->Requests()) {
    request->SetLifetime(lifetime);
  }
}

}  // namespace permissions
