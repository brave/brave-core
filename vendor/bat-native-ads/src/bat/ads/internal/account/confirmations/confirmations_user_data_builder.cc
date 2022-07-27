/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations_user_data_builder.h"

#include "base/check_op.h"
#include "bat/ads/internal/account/user_data/build_channel_user_data.h"
#include "bat/ads/internal/account/user_data/catalog_user_data.h"
#include "bat/ads/internal/account/user_data/conversion_user_data.h"
#include "bat/ads/internal/account/user_data/created_at_timestamp_user_data.h"
#include "bat/ads/internal/account/user_data/locale_user_data.h"
#include "bat/ads/internal/account/user_data/mutated_user_data.h"
#include "bat/ads/internal/account/user_data/odyssey_user_data.h"
#include "bat/ads/internal/account/user_data/platform_user_data.h"
#include "bat/ads/internal/account/user_data/rotating_hash_user_data.h"
#include "bat/ads/internal/account/user_data/studies_user_data.h"
#include "bat/ads/internal/account/user_data/system_timestamp_user_data.h"
#include "bat/ads/internal/account/user_data/version_number_user_data.h"

namespace ads {

ConfirmationsUserDataBuilder::ConfirmationsUserDataBuilder(
    const base::Time time,
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type)
    : time_(time),
      creative_instance_id_(creative_instance_id),
      confirmation_type_(confirmation_type) {
  DCHECK(!creative_instance_id_.empty());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type_.value());
}

ConfirmationsUserDataBuilder::~ConfirmationsUserDataBuilder() = default;

void ConfirmationsUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  user_data::GetConversion(
      creative_instance_id_, confirmation_type_,
      [=](base::Value::Dict user_data) {
        base::Value::Dict build_channel_user_data =
            user_data::GetBuildChannel();
        user_data.Merge(std::move(build_channel_user_data));

        base::Value::Dict catalog_user_data = user_data::GetCatalog();
        user_data.Merge(std::move(catalog_user_data));

        base::Value::Dict created_at_timestamp_user_data =
            user_data::GetCreatedAtTimestamp(time_);
        user_data.Merge(std::move(created_at_timestamp_user_data));

        base::Value::Dict locale_user_data = user_data::GetLocale();
        user_data.Merge(std::move(locale_user_data));

        base::Value::Dict mutated_user_data = user_data::GetMutated();
        user_data.Merge(std::move(mutated_user_data));

        base::Value::Dict odyssey_user_data = user_data::GetOdyssey();
        user_data.Merge(std::move(odyssey_user_data));

        base::Value::Dict platform_user_data = user_data::GetPlatform();
        user_data.Merge(std::move(platform_user_data));

        base::Value::Dict rotating_hash_user_data =
            user_data::GetRotatingHash(creative_instance_id_);
        user_data.Merge(std::move(rotating_hash_user_data));

        base::Value::Dict studies_user_data = user_data::GetStudies();
        user_data.Merge(std::move(studies_user_data));

        base::Value::Dict system_timestamp_user_data =
            user_data::GetSystemTimestamp();
        user_data.Merge(std::move(system_timestamp_user_data));

        base::Value::Dict version_number_user_data =
            user_data::GetVersionNumber();
        user_data.Merge(std::move(version_number_user_data));

        callback(user_data);
      });
}

}  // namespace ads
