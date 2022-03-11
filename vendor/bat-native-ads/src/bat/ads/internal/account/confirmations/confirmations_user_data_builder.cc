/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations_user_data_builder.h"

#include "base/check_op.h"
#include "base/values.h"
#include "bat/ads/internal/account/user_data/build_channel_user_data.h"
#include "bat/ads/internal/account/user_data/conversion_user_data.h"
#include "bat/ads/internal/account/user_data/locale_user_data.h"
#include "bat/ads/internal/account/user_data/odyssey_user_data.h"
#include "bat/ads/internal/account/user_data/platform_user_data.h"
#include "bat/ads/internal/account/user_data/studies_user_data.h"

namespace ads {

ConfirmationsUserDataBuilder::ConfirmationsUserDataBuilder(
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type)
    : creative_instance_id_(creative_instance_id),
      confirmation_type_(confirmation_type) {
  DCHECK(!creative_instance_id_.empty());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type_.value());
}

ConfirmationsUserDataBuilder::~ConfirmationsUserDataBuilder() = default;

void ConfirmationsUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  user_data::GetConversion(
      creative_instance_id_, confirmation_type_,
      [callback](base::Value user_data) {
        const base::DictionaryValue platform_user_data =
            user_data::GetPlatform();
        user_data.MergeDictionary(&platform_user_data);

        const base::DictionaryValue build_channel_user_data =
            user_data::GetBuildChannel();
        user_data.MergeDictionary(&build_channel_user_data);

        const base::DictionaryValue locale_user_data = user_data::GetLocale();
        user_data.MergeDictionary(&locale_user_data);

        const base::DictionaryValue studies_user_data = user_data::GetStudies();
        user_data.MergeDictionary(&studies_user_data);

        const base::DictionaryValue odyssey_user_data = user_data::GetOdyssey();
        user_data.MergeDictionary(&odyssey_user_data);

        callback(user_data);
      });
}

}  // namespace ads
