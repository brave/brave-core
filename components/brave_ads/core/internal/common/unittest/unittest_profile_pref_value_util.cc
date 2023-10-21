/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_storage.h"

namespace brave_ads {

base::Value GetProfilePrefValue(const std::string& path) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  const PrefInfo& pref = ProfilePref(path);
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

}  // namespace brave_ads
