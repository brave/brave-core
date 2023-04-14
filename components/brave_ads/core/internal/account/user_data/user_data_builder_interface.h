/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_USER_DATA_BUILDER_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_USER_DATA_BUILDER_INTERFACE_H_

#include "base/functional/callback_forward.h"
#include "base/values.h"

namespace brave_ads {

// TODO(tmancey)
using UserDataBuilderCallback = base::OnceCallback<void(base::Value::Dict)>;

class UserDataBuilderInterface {
 public:
  virtual ~UserDataBuilderInterface() = default;

  virtual void Build(UserDataBuilderCallback callback) const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_USER_DATA_BUILDER_INTERFACE_H_
