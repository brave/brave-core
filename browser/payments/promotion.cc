/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/promotion.h"

namespace payments {

  Promotion::Promotion() :
      amount(0.0) {
  }

  Promotion::~Promotion() { }

  Promotion::Promotion(const Promotion &properties) {
    promotionId = properties.promotionId;
    amount = properties.amount;
  }

}  // namespace payments
