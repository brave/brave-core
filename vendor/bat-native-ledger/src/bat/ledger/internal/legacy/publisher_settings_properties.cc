/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/publisher_settings_properties.h"
#include "bat/ledger/internal/static_values.h"

namespace ledger {

PublisherSettingsProperties::PublisherSettingsProperties()
    : min_page_time_before_logging_a_visit(8),
      min_visits_for_publisher_relevancy(1),
      allow_non_verified_sites_in_list(true),
      allow_contribution_to_videos(true) {}

PublisherSettingsProperties::PublisherSettingsProperties(
    const PublisherSettingsProperties& properties) {
  min_page_time_before_logging_a_visit =
      properties.min_page_time_before_logging_a_visit;
  min_visits_for_publisher_relevancy =
      properties.min_visits_for_publisher_relevancy;
  allow_non_verified_sites_in_list =
      properties.allow_non_verified_sites_in_list;
  allow_contribution_to_videos = properties.allow_contribution_to_videos;
  monthly_balances = properties.monthly_balances;
  processed_pending_publishers = properties.processed_pending_publishers;
}

PublisherSettingsProperties::~PublisherSettingsProperties() = default;

bool PublisherSettingsProperties::operator==(
    const PublisherSettingsProperties& rhs) const {
  return
      min_page_time_before_logging_a_visit ==
          rhs.min_page_time_before_logging_a_visit &&
      min_visits_for_publisher_relevancy ==
          rhs.min_visits_for_publisher_relevancy &&
      allow_non_verified_sites_in_list ==
          rhs.allow_non_verified_sites_in_list &&
      allow_contribution_to_videos == rhs.allow_contribution_to_videos &&
      monthly_balances == rhs.monthly_balances &&
      processed_pending_publishers == rhs.processed_pending_publishers;
}

bool PublisherSettingsProperties::operator!=(
    const PublisherSettingsProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
