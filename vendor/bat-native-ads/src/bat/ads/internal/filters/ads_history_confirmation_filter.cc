/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>

#include "bat/ads/internal/filters/ads_history_confirmation_filter.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/ads_history.h"

namespace ads {

bool IsConfirmationTypeOfInterest(
    const ConfirmationType& confirmation_type) {
  bool is_of_interest = false;

  switch (confirmation_type.value()) {
    case ConfirmationType::Value::kClicked:
    case ConfirmationType::Value::kViewed:
    case ConfirmationType::Value::kDismissed: {
      is_of_interest = true;
      break;
    }
    case ConfirmationType::Value::kUnknown:
    case ConfirmationType::Value::kLanded:
    case ConfirmationType::Value::kFlagged:
    case ConfirmationType::Value::kUpvoted:
    case ConfirmationType::Value::kDownvoted: {
      is_of_interest = false;
      break;
    }
  }
  return is_of_interest;
}

bool DoesConfirmationTypeATrumpB(
    const ConfirmationType& confirmation_type_a,
    const ConfirmationType& confirmation_type_b) {
  bool does_type_a_trump_type_b = false;

  switch (confirmation_type_a.value()) {
    case ConfirmationType::Value::kClicked: {
      switch (confirmation_type_b.value()) {
        case ConfirmationType::Value::kClicked:
        case ConfirmationType::Value::kViewed:
        case ConfirmationType::Value::kDismissed: {
          does_type_a_trump_type_b = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ConfirmationType::Value::kViewed: {
      switch (confirmation_type_b.value()) {
        case ConfirmationType::Value::kViewed:
        case ConfirmationType::Value::kDismissed: {
          does_type_a_trump_type_b = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ConfirmationType::Value::kDismissed: {
      switch (confirmation_type_b.value()) {
        case ConfirmationType::Value::kDismissed: {
          does_type_a_trump_type_b = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
  }

  return does_type_a_trump_type_b;
}

AdsHistoryConfirmationFilter::~AdsHistoryConfirmationFilter() = default;

std::deque<AdHistory> AdsHistoryConfirmationFilter::Apply(
    const std::deque<AdHistory>& history) const {

  std::map<std::string, AdHistory> filtered_ad_history;

  for (const AdHistory& entry : history) {
    if (!IsConfirmationTypeOfInterest(entry.ad_content.ad_action)) {
      continue;
    }
    if (filtered_ad_history.count(entry.ad_content.uuid) != 0) {
      const AdHistory& check_entry =
          filtered_ad_history[entry.ad_content.uuid];

      if (entry.timestamp_in_seconds >= check_entry.timestamp_in_seconds) {
        if (DoesConfirmationTypeATrumpB(entry.ad_content.ad_action,
            check_entry.ad_content.ad_action)) {
          filtered_ad_history[entry.ad_content.uuid] = entry;
        }
      }
    } else {
      filtered_ad_history[entry.ad_content.uuid] = entry;
    }
  }

  std::deque<AdHistory> filtered_history;

  for (const auto& ad_history : filtered_ad_history) {
    filtered_history.push_back(ad_history.second);
  }

  return filtered_history;
}

}  // namespace ads
