
/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_DESCENDING_SORT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_DESCENDING_SORT_H_

#include "brave/components/brave_ads/core/internal/conversions/sorts/conversions_sort_interface.h"

namespace brave_ads {

class ConversionsDescendingSort final : public ConversionsSortInterface {
 public:
  ConversionList Apply(const ConversionList& conversions) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_DESCENDING_SORT_H_
