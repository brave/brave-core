/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/conversion_user_data.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/user_data/conversion_user_data_builder.h"

namespace ads::user_data {

void GetConversion(const std::string& creative_instance_id,
                   const ConfirmationType& confirmation_type,
                   ConversionCallback callback) {
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type.value());

  if (confirmation_type != ConfirmationType::kConversion) {
    std::move(callback).Run(base::Value::Dict());
    return;
  }

  builder::BuildConversion(
      creative_instance_id,
      base::BindOnce(
          [](ConversionCallback callback, base::Value::Dict user_data) {
            std::move(callback).Run(std::move(user_data));
          },
          std::move(callback)));
}

}  // namespace ads::user_data
