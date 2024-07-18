/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_value_test_util_internal.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/pref_value_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_storage_test_util_internal.h"

namespace brave_ads::test {

base::Value GetProfilePrefValue(const std::string& path) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  const PrefValueInfo& pref = ProfilePref(path);
  if (!pref.value) {
    return pref.default_value.Clone();
  }

  return pref.value->Clone();
}

base::Value GetDefaultProfilePrefValue(const std::string& path) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  return ProfilePref(path).default_value.Clone();
}

void ClearProfilePrefValue(const std::string& path) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to clear an unregistered pref: " << path;

  ProfilePref(path).value.reset();
}

bool HasProfilePrefPathValue(const std::string& path) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  return !!ProfilePref(path).value;
}

}  // namespace brave_ads::test
