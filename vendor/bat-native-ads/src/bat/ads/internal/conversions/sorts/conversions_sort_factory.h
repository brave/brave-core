/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_FACTORY_H_

#include <memory>

#include "bat/ads/internal/conversions/conversion_info.h"

namespace ads {

class ConversionsSort;

class ConversionsSortFactory {
 public:
  static std::unique_ptr<ConversionsSort> Build(
      const ConversionInfo::SortType type);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_FACTORY_H_
