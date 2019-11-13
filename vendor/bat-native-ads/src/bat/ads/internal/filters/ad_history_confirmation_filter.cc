/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <memory>
#include <string>

#include "bat/ads/internal/filters/ad_history_confirmation_filter.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/ad_history_detail.h"

#include "bat/ads/ads_history.h"

namespace ads {

bool IsConfirmationTypeOfInterest(
    const ConfirmationType& confirmation_type) {
  bool is_of_interest = false;

  switch (confirmation_type.value()) {
    case ConfirmationType::Value::CLICK:
    case ConfirmationType::Value::VIEW:
    case ConfirmationType::Value::DISMISS: {
      is_of_interest = true;
      break;
    }
    case ConfirmationType::Value::UNKNOWN:
    case ConfirmationType::Value::LANDED:
    case ConfirmationType::Value::FLAG:
    case ConfirmationType::Value::UPVOTE:
    case ConfirmationType::Value::DOWNVOTE: {
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
    case ConfirmationType::Value::CLICK: {
      switch (confirmation_type_b.value()) {
        case ConfirmationType::Value::CLICK:
        case ConfirmationType::Value::VIEW:
        case ConfirmationType::Value::DISMISS: {
          does_type_a_trump_type_b = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ConfirmationType::Value::VIEW: {
      switch (confirmation_type_b.value()) {
        case ConfirmationType::Value::VIEW:
        case ConfirmationType::Value::DISMISS: {
          does_type_a_trump_type_b = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ConfirmationType::Value::DISMISS: {
      switch (confirmation_type_b.value()) {
        case ConfirmationType::Value::DISMISS: {
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

AdHistoryConfirmationFilter::~AdHistoryConfirmationFilter() = default;

std::deque<AdHistoryDetail> AdHistoryConfirmationFilter::ApplyFilter(
    const std::deque<AdHistoryDetail>& ad_history_details) const {

  std::map<std::string, AdHistoryDetail> filtered_ad_history;

  for (const AdHistoryDetail& ad_history_detail : ad_history_details) {
    if (!IsConfirmationTypeOfInterest(ad_history_detail.ad_content.ad_action)) {
      continue;
    }
    if (filtered_ad_history.count(ad_history_detail.ad_content.uuid) != 0) {
      const AdHistoryDetail& check_ad_history_detail =
          filtered_ad_history[ad_history_detail.ad_content.uuid];

      if (ad_history_detail.timestamp_in_seconds >=
          check_ad_history_detail.timestamp_in_seconds) {
        if (DoesConfirmationTypeATrumpB(ad_history_detail.ad_content.ad_action,
            check_ad_history_detail.ad_content.ad_action)) {
          filtered_ad_history[ad_history_detail.ad_content.uuid] =
              ad_history_detail;
        }
      }
    } else {
      filtered_ad_history[ad_history_detail.ad_content.uuid] =
          ad_history_detail;
    }
  }

  std::deque<AdHistoryDetail> filtered_ad_history_details;

  for (const auto& ad_history : filtered_ad_history) {
    filtered_ad_history_details.push_back(ad_history.second);
  }

  return filtered_ad_history_details;
}

}  // namespace ads
