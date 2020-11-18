/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/purchase_intent_classifier.h"
#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const uint16_t kMaximumPurchaseIntentSegments = 3;
}  // namespace

AdTargeting::AdTargeting(
    ad_targeting::contextual::PageClassifier* page_classifier,
    ad_targeting::behavioral::PurchaseIntentClassifier*
        purchase_intent_classifier)
    : page_classifier_(page_classifier),
      purchase_intent_classifier_(purchase_intent_classifier) {
  DCHECK(page_classifier_);
  DCHECK(purchase_intent_classifier_);
}

AdTargeting::~AdTargeting() = default;

CategoryList AdTargeting::GetCategories() const {
  const CategoryList page_classification_categories =
      GetPageClassificationCategories();

  const CategoryList purchase_intent_categories = GetPurchaseIntentCategories();

  CategoryList categories = page_classification_categories;

  categories.insert(categories.end(), purchase_intent_categories.begin(),
      purchase_intent_categories.end());

  return categories;
}

///////////////////////////////////////////////////////////////////////////////

CategoryList AdTargeting::GetPageClassificationCategories() const {
  return page_classifier_->GetWinningCategories();
}

CategoryList AdTargeting::GetPurchaseIntentCategories() const {
  const PurchaseIntentSignalSegmentHistoryMap purchase_intent_signal_history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const CategoryList categories =
      purchase_intent_classifier_->GetWinningCategories(
          purchase_intent_signal_history, kMaximumPurchaseIntentSegments);

  return categories;
}

}  // namespace ads
