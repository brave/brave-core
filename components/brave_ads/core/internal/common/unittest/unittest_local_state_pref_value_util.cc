/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_value_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_storage.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_info.h"

namespace brave_ads {

base::Value GetLocalStatePrefValue(const std::string& path) {
  CHECK(HasLocalStatePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  const PrefInfo& pref = LocalStatePref(path);
  if (!pref.value) {
    return pref.default_value.Clone();
  }

  return pref.value->Clone();
}

base::Value GetDefaultLocalStatePrefValue(const std::string& path) {
  CHECK(HasLocalStatePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  return LocalStatePref(path).default_value.Clone();
}

void ClearLocalStatePrefValue(const std::string& path) {
  CHECK(HasLocalStatePref(path))
      << "Check failed. Trying to clear an unregistered pref: " << path;

  LocalStatePref(path).value.reset();
}

bool HasLocalStatePrefPathValue(const std::string& path) {
  CHECK(HasLocalStatePref(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  return !!LocalStatePref(path).value;
}

}  // namespace brave_ads
