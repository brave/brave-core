/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_dto_user_data_builder.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_build_channel_dto_user_data.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_conversion_dto_user_data.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_locale_dto_user_data.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_platform_dto_user_data.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_studies_dto_user_data.h"

namespace ads {
namespace dto {
namespace user_data {

void Build(const std::string& creative_instance_id,
           const ConfirmationType& confirmation_type,
           Callback callback) {
  base::DictionaryValue user_data;

  const base::DictionaryValue platform_user_data = GetPlatform();
  user_data.MergeDictionary(&platform_user_data);

  const base::DictionaryValue build_channel_user_data = GetBuildChannel();
  user_data.MergeDictionary(&build_channel_user_data);

  const base::DictionaryValue locale_user_data = GetLocale();
  user_data.MergeDictionary(&locale_user_data);

  const base::DictionaryValue studies_user_data = GetStudies();
  user_data.MergeDictionary(&studies_user_data);

  if (confirmation_type != ConfirmationType::kConversion) {
    callback(user_data.Clone());
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/14429): Remove
  // serialization/deserialization from DTO
  std::string user_data_as_string;
  base::JSONWriter::Write(user_data, &user_data_as_string);

  database::table::ConversionQueue database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const Result result, const std::string& creative_instance_id,
          const ConversionQueueItemList& conversion_queue_items) {
        if (result != Result::SUCCESS) {
          BLOG(1, "Failed to get conversion queue");
          return;
        }

        if (conversion_queue_items.empty()) {
          BLOG(1, "Conversion queue is empty");
          return;
        }

        const ConversionQueueItemInfo conversion_queue_item =
            conversion_queue_items.front();

        const base::DictionaryValue conversion_user_data =
            GetConversion(conversion_queue_item);

        absl::optional<base::Value> user_data_as_value =
            base::JSONReader::Read(user_data_as_string);
        if (!user_data_as_value || !user_data_as_value->is_dict()) {
          return;
        }

        base::DictionaryValue* user_data = nullptr;
        if (!user_data_as_value->GetAsDictionary(&user_data)) {
          NOTREACHED();
          return;
        }

        user_data->MergeDictionary(&conversion_user_data);

        callback(user_data->Clone());
      });
}

}  // namespace user_data
}  // namespace dto
}  // namespace ads
