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
  DCHECK(transaction_.IsValid());
}

void ConfirmationUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  user_data::GetConversion(
      transaction_.creative_instance_id, transaction_.confirmation_type,
      base::BindOnce(&ConfirmationUserDataBuilder::OnGetConversion,
                     base::Unretained(this), std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationUserDataBuilder::OnGetConversion(
    UserDataBuilderCallback callback,
    base::Value::Dict user_data) const {
  user_data.Merge(user_data::GetBuildChannel());
  user_data.Merge(user_data::GetCatalog());
  user_data.Merge(user_data::GetCreatedAtTimestamp(transaction_));
  user_data.Merge(user_data::GetLocale());
  user_data.Merge(user_data::GetMutated());
  user_data.Merge(user_data::GetPlatform());
  user_data.Merge(user_data::GetRotatingHash(transaction_));
  user_data.Merge(user_data::GetSegment(transaction_));
  user_data.Merge(user_data::GetStudies());
  user_data.Merge(user_data::GetVersionNumber());

  std::move(callback).Run(std::move(user_data));
}

}  // namespace brave_ads
