/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_service.h"

#include "base/files/file_path.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/util/values/values_util.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"

namespace brave_shields {

namespace {
bool SkipGURLField(base::StringPiece value, GURL* field) {
  return true;
}

bool ParseTimeValue(const base::Value* value, base::Time* field) {
  auto time = util::ValueToTime(value);
  if (!time) {
    return false;
  }
  *field = *time;
  return true;
}
}  // namespace

void SubscriptionInfo::RegisterJSONConverter(
    base::JSONValueConverter<SubscriptionInfo>* converter) {
  // The `subscription_url` field is skipped, as it's not stored within the
  // JSON value and should be populated externally.
  converter->RegisterCustomField<GURL>(
      "subscription_url", &SubscriptionInfo::subscription_url, &SkipGURLField);
  converter->RegisterCustomValueField<base::Time>(
      "last_update_attempt", &SubscriptionInfo::last_update_attempt,
      &ParseTimeValue);
  converter->RegisterCustomValueField<base::Time>(
      "last_successful_update_attempt",
      &SubscriptionInfo::last_successful_update_attempt, &ParseTimeValue);
  converter->RegisterBoolField("enabled", &SubscriptionInfo::enabled);
}

AdBlockSubscriptionService::AdBlockSubscriptionService(
    const SubscriptionInfo& info,
    const base::FilePath list_file,
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate),
      subscription_url_(info.subscription_url),
      list_file_(list_file),
      load_on_start_(info.last_successful_update_attempt != base::Time::Min()),
      initialized_(false) {}

AdBlockSubscriptionService::~AdBlockSubscriptionService() {}

bool AdBlockSubscriptionService::Init() {
  if (!AdBlockBaseService::Init())
    return false;

  // if we already have local data, go ahead and load it
  if (load_on_start_) {
    load_on_start_ = false;
    ReloadList();
    // return false so we don't set the component to initialized until the data
    // is loaded
    return false;
  }

  return initialized_;
}

void AdBlockSubscriptionService::ReloadList() {
  GetDATFileData(list_file_, false,
                 base::BindOnce(&AdBlockSubscriptionService::OnListLoaded,
                                weak_factory_.GetWeakPtr()));
}

void AdBlockSubscriptionService::OnListLoaded() {
  Start();
  initialized_ = true;
}

}  // namespace brave_shields
