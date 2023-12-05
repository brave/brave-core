/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_constants.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/queue/conversion_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"

namespace brave_ads {

void BuildConversionUserData(const std::string& creative_instance_id,
                             BuildUserDataCallback callback) {
  CHECK(!creative_instance_id.empty());

  const database::table::ConversionQueue database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          [](BuildUserDataCallback callback, const bool success,
             const std::string& /*creative_instance_id*/,
             const ConversionQueueItemList& conversion_queue_items) {
            if (!success) {
              return std::move(callback).Run(/*user_data=*/{});
            }

            if (conversion_queue_items.empty()) {
              return std::move(callback).Run(/*user_data=*/{});
            }

            const ConversionQueueItemInfo& conversion_queue_item =
                conversion_queue_items.front();

            base::Value::List list;

            // Conversion.
            list.Append(
                BuildConversionActionTypeUserData(conversion_queue_item));

            // Verifiable conversion.
            std::optional<base::Value::Dict> verifiable_conversion_user_data =
                MaybeBuildVerifiableConversionUserData(conversion_queue_item);
            if (verifiable_conversion_user_data) {
              list.Append(std::move(*verifiable_conversion_user_data));
            }

            auto user_data =
                base::Value::Dict().Set(kConversionKey, std::move(list));

            std::move(callback).Run(std::move(user_data));
          },
          std::move(callback)));
}

}  // namespace brave_ads
