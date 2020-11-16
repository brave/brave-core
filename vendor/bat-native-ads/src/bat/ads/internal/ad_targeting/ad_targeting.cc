/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/purchase_intent_classifier.h"
#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const uint16_t kMaximumPurchaseIntentSegments = 3;
}  // namespace

AdTargeting::AdTargeting(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdTargeting::~AdTargeting() = default;

CategoryList AdTargeting::GetWinningCategories() const {
  const CategoryList page_classification_winning_categories =
      GetPageClassificationWinningCategories();

  const CategoryList purchase_intent_winning_categories =
      GetPurchaseIntentWinningCategories();

  CategoryList winning_categories = page_classification_winning_categories;

  winning_categories.insert(winning_categories.end(),
      purchase_intent_winning_categories.begin(),
          purchase_intent_winning_categories.end());

  return winning_categories;
}

///////////////////////////////////////////////////////////////////////////////

CategoryList AdTargeting::GetPageClassificationWinningCategories() const {
  return ads_->get_page_classifier()->GetWinningCategories();
}

CategoryList AdTargeting::GetPurchaseIntentWinningCategories() const {
  const PurchaseIntentSignalSegmentHistoryMap purchase_intent_signal_history =
      ads_->get_client()->GetPurchaseIntentSignalHistory();

  const CategoryList categories =
      ads_->get_purchase_intent_classifier()->GetWinningCategories(
          purchase_intent_signal_history, kMaximumPurchaseIntentSegments);

  return categories;
}

}  // namespace ads
