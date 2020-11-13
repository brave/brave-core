/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_H_
#define BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_H_

#include <string>
#include <vector>

namespace ads {

namespace ad_targeting {

namespace contextual {
class PageClassifier;
}  // namespace contextual

namespace behavioral {
class PurchaseIntentClassifier;
}  // namespace behavioral

}  // namespace ad_targeting

using CategoryList = std::vector<std::string>;

class AdTargeting {
 public:
  AdTargeting(
      ad_targeting::contextual::PageClassifier* page_classifier,
      ad_targeting::behavioral::PurchaseIntentClassifier*
          purchase_intent_classifier);

  ~AdTargeting();

  CategoryList GetCategories() const;

 private:
  CategoryList GetPageClassificationCategories() const;

  CategoryList GetPurchaseIntentCategories() const;

  ad_targeting::contextual::PageClassifier* page_classifier_;  // NOT OWNED
  ad_targeting::behavioral::PurchaseIntentClassifier*
      purchase_intent_classifier_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_H_
