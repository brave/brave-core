/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_ASCENDING_SORT_H_
#define BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_ASCENDING_SORT_H_

#include "bat/ads/internal/conversions/sorts/conversions_sort.h"

namespace ads {

class ConversionsAscendingSort : public ConversionsSort {
 public :
  ConversionsAscendingSort();
  ~ConversionsAscendingSort() override;

  ConversionList Apply(
      const ConversionList& list) const override;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONVERSIONS_SORTS_CONVERSIONS_ASCENDING_SORT_H_
