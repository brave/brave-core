/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_H_

namespace brave_ads {

enum class ConversionActionType {
  kUndefined = 0,

  // View-through conversions occur when a user is exposed to an ad viewed
  // impression and later completes a desired action, such as making a purchase
  // or filling out a form. The conversion is attributed to the ad view
  // impression rather than a direct click.
  kViewThrough,

  // Click-through conversions occur when a user clicks on an ad and later
  // completes a desired action, such as making a purchase or filling out a
  // form. Click-through conversions should take priority over view-through.
  kClickThrough
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_H_
