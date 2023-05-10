/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_BUILDER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/values.h"

namespace brave_ads {

using BuildVerifiableConversionUserDataCallback =
    base::OnceCallback<void(base::Value::Dict user_data)>;

void BuildVerifiableConversionUserData(
    const std::string& creative_instance_id,
    BuildVerifiableConversionUserDataCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_BUILDER_H_
