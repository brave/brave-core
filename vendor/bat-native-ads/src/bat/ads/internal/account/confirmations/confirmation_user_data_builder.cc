/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_user_data_builder.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "bat/ads/internal/account/user_data/build_channel_user_data.h"
#include "bat/ads/internal/account/user_data/catalog_user_data.h"
#include "bat/ads/internal/account/user_data/conversion_user_data.h"
#include "bat/ads/internal/account/user_data/created_at_timestamp_user_data.h"
#include "bat/ads/internal/account/user_data/diagnostic_id_user_data.h"
#include "bat/ads/internal/account/user_data/locale_user_data.h"
#include "bat/ads/internal/account/user_data/mutated_user_data.h"
#include "bat/ads/internal/account/user_data/odyssey_user_data.h"
#include "bat/ads/internal/account/user_data/platform_user_data.h"
#include "bat/ads/internal/account/user_data/rotating_hash_user_data.h"
#include "bat/ads/internal/account/user_data/studies_user_data.h"
#include "bat/ads/internal/account/user_data/system_timestamp_user_data.h"
#include "bat/ads/internal/account/user_data/version_number_user_data.h"

namespace ads {

ConfirmationUserDataBuilder::ConfirmationUserDataBuilder(
    const base::Time created_at,
    std::string creative_instance_id,
    const ConfirmationType& confirmation_type)
    : created_at_(created_at),
      creative_instance_id_(std::move(creative_instance_id)),
      confirmation_type_(confirmation_type) {
  DCHECK(!creative_instance_id_.empty());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type_.value());
}

void ConfirmationUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  user_data::GetConversion(
      creative_instance_id_, confirmation_type_,
      base::BindOnce(&ConfirmationUserDataBuilder::OnGetConversion,
                     base::Unretained(this), std::move(callback)));
}

void ConfirmationUserDataBuilder::OnGetConversion(
    UserDataBuilderCallback callback,
    base::Value::Dict user_data) const {
  user_data.Merge(user_data::GetBuildChannel());
  user_data.Merge(user_data::GetCatalog());
  user_data.Merge(user_data::GetCreatedAtTimestamp(created_at_));
  user_data.Merge(user_data::GetDiagnosticId());
  user_data.Merge(user_data::GetLocale());
  user_data.Merge(user_data::GetMutated());
  user_data.Merge(user_data::GetOdyssey());
  user_data.Merge(user_data::GetPlatform());
  user_data.Merge(user_data::GetRotatingHash(creative_instance_id_));
  user_data.Merge(user_data::GetStudies());
  user_data.Merge(user_data::GetSystemTimestamp());
  user_data.Merge(user_data::GetVersionNumber());

  std::move(callback).Run(std::move(user_data));
}

}  // namespace ads
