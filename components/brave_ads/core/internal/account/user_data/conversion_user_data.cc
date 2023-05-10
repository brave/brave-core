/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data_builder.h"

namespace brave_ads {

void BuildConversionUserData(const std::string& creative_instance_id,
                             const ConfirmationType& confirmation_type,
                             BuildConversionUserDataCallback callback) {
  CHECK(!creative_instance_id.empty());
  CHECK_NE(ConfirmationType::kUndefined, confirmation_type);

  if (confirmation_type != ConfirmationType::kConversion) {
    return std::move(callback).Run(base::Value::Dict());
  }

  BuildVerifiableConversionUserData(
      creative_instance_id,
      base::BindOnce(
          [](BuildVerifiableConversionUserDataCallback callback,
             base::Value::Dict user_data) {
            std::move(callback).Run(std::move(user_data));
          },
          std::move(callback)));
}

}  // namespace brave_ads
