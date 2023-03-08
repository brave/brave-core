/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_FACTORY_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/conversions/sorts/conversion_sort_types.h"
#include "brave/components/brave_ads/core/internal/conversions/sorts/conversions_sort_interface.h"

namespace ads {

class ConversionsSortFactory final {
 public:
  static std::unique_ptr<ConversionsSortInterface> Build(
      ConversionSortType type);
};

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_SORT_FACTORY_H_
