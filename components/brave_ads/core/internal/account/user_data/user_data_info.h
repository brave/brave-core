/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_USER_DATA_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_USER_DATA_INFO_H_

#include "base/values.h"

namespace brave_ads {

struct UserDataInfo final {
  UserDataInfo();

  UserDataInfo(const UserDataInfo&);
  UserDataInfo& operator=(const UserDataInfo&);

  UserDataInfo(UserDataInfo&&) noexcept;
  UserDataInfo& operator=(UserDataInfo&&) noexcept;

  ~UserDataInfo();

  bool operator==(const UserDataInfo&) const = default;

  // User data that may change.
  base::Value::Dict dynamic;

  // User data that remains constant.
  base::Value::Dict fixed;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_USER_DATA_INFO_H_
