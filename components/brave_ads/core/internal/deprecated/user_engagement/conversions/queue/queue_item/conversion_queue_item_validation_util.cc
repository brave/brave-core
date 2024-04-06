/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_validation_util.h"

#include <vector>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

namespace {

constexpr char kAdTypeFieldName[] = "ad_type";
constexpr char kCreativeInstanceIdFieldName[] = "creative_instance_id";
constexpr char kCreativeSetIdFieldName[] = "creative_set_id";
constexpr char kCampaignIdFieldName[] = "campaign_id";
constexpr char kAdvertiserIdFieldName[] = "advertiser_id";
constexpr char kActionTypeFieldName[] = "action_type";
constexpr char kProcessAtFieldName[] = "process_at";
constexpr char kSeparator[] = ",";

std::vector<std::string> GetInvalidFieldsNamesList(
    const ConversionInfo& conversion_item) {
  std::vector<std::string> invalid_fields;

  if (conversion_item.ad_type == AdType::kUndefined) {
    invalid_fields.emplace_back(kAdTypeFieldName);
  }

  if (conversion_item.creative_instance_id.empty()) {
    invalid_fields.emplace_back(kCreativeInstanceIdFieldName);
  }

  if (conversion_item.creative_set_id.empty()) {
    invalid_fields.emplace_back(kCreativeSetIdFieldName);
  }

  if (conversion_item.campaign_id.empty()) {
    invalid_fields.emplace_back(kCampaignIdFieldName);
  }

  if (conversion_item.advertiser_id.empty()) {
    invalid_fields.emplace_back(kAdvertiserIdFieldName);
  }

  if (conversion_item.action_type == ConversionActionType::kUndefined) {
    invalid_fields.emplace_back(kActionTypeFieldName);
  }

  return invalid_fields;
}

std::vector<std::string> GetInvalidFieldsNamesList(
    const ConversionQueueItemInfo& conversion_queue_item) {
  std::vector<std::string> invalid_fields =
      GetInvalidFieldsNamesList(conversion_queue_item.conversion);
  if (conversion_queue_item.process_at.is_null()) {
    invalid_fields.emplace_back(kProcessAtFieldName);
  }
  return invalid_fields;
}

}  // namespace

std::string GetConversionQueueItemInvalidFieldsNames(
    const ConversionQueueItemInfo& conversion_queue_item) {
  return base::JoinString(GetInvalidFieldsNamesList(conversion_queue_item),
                          kSeparator);
}

}  // namespace brave_ads
