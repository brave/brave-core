/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_H_
#define BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_H_

#include <string>
#include <vector>

namespace ads {

class AdsImpl;

using CategoryList = std::vector<std::string>;

class AdTargeting {
 public:
  AdTargeting(
      AdsImpl* ads);

  ~AdTargeting();

  CategoryList GetWinningCategories() const;

 private:
  CategoryList GetPageClassificationWinningCategories() const;

  CategoryList GetPurchaseIntentWinningCategories() const;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_H_
