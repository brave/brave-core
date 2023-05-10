/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_user_data_builder.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/user_data/build_channel_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/catalog_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/created_at_timestamp_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/locale_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/mutated_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/platform_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/rotating_hash_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/segment_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/studies_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/version_number_user_data.h"

namespace brave_ads {

ConfirmationUserDataBuilder::ConfirmationUserDataBuilder(
    TransactionInfo transaction)
    : transaction_(std::move(transaction)) {
  CHECK(transaction_.IsValid());
}

ConfirmationUserDataBuilder::~ConfirmationUserDataBuilder() = default;

void ConfirmationUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  BuildConversionUserData(
      transaction_.creative_instance_id, transaction_.confirmation_type,
      base::BindOnce(&ConfirmationUserDataBuilder::GetConversionCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationUserDataBuilder::GetConversionCallback(
    UserDataBuilderCallback callback,
    base::Value::Dict user_data) const {
  user_data.Merge(BuildBuildChannelUserData());
  user_data.Merge(BuildCatalogUserData());
  user_data.Merge(BuildCreatedAtTimestampUserData(transaction_));
  user_data.Merge(BuildLocaleUserData());
  user_data.Merge(BuildMutatedUserData());
  user_data.Merge(BuildPlatformUserData());
  user_data.Merge(BuildRotatingHashUserData(transaction_));
  user_data.Merge(BuildSegmentUserData(transaction_));
  user_data.Merge(BuildStudiesUserData());
  user_data.Merge(BuildVersionNumberUserData());

  std::move(callback).Run(std::move(user_data));
}

}  // namespace brave_ads
