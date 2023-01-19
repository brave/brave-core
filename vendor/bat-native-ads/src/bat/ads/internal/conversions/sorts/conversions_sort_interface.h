/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_INTERFACE_H_

#include "bat/ads/internal/conversions/conversion_info.h"

namespace ads {

class ConversionsSortInterface {
 public:
  virtual ~ConversionsSortInterface() = default;

  virtual ConversionList Apply(const ConversionList& conversions) const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_INTERFACE_H_
