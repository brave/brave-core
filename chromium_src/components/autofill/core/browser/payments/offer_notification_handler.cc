/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/payments/offer_notification_handler.h"

#include "components/autofill/core/browser/data_model/autofill_offer_data.h"
#include "components/autofill/core/browser/payments/autofill_offer_manager.h"

namespace autofill {

namespace {

// This replicates the functionality that the removed upstream flag
// kAutofillEnableOfferNotificationForPromoCodes used to have.
bool BraveIsOfferValid(const AutofillOfferData* offer) {
  if (!offer) {
    return false;
  }

  if (offer->IsPromoCodeOffer()) {
    return false;
  }

  return true;
}

}  // namespace

}  // namespace autofill

#define IsUrlEligible(URL) \
  IsUrlEligible(URL) && BraveIsOfferValid(offer_manager_->GetOfferForUrl(URL))

#include "src/components/autofill/core/browser/payments/offer_notification_handler.cc"
#undef IsUrlEligible
