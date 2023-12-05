/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_BUILD_USER_DATA_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_BUILD_USER_DATA_CALLBACK_H_

#include "base/functional/callback.h"
#include "base/values.h"

namespace brave_ads {

using BuildUserDataCallback =
    base::OnceCallback<void(/*user_data*/ base::Value::Dict)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_BUILD_USER_DATA_CALLBACK_H_
