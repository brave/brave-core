// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_autofill_data_manager_extras.h"

#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "ios/web_view/internal/autofill/cwv_autofill_data_manager_internal.h"
#include "ios/web_view/internal/autofill/cwv_credit_card_internal.h"

@implementation CWVAutofillDataManager (Extras)

- (void)addCreditCard:(CWVCreditCard*)creditCard {
  [self personalDataManager]->payments_data_manager().AddCreditCard(
      *creditCard.internalCard);
}

- (void)updateCreditCard:(CWVCreditCard*)creditCard {
  [self personalDataManager]->payments_data_manager().UpdateCreditCard(
      *creditCard.internalCard);
}

- (void)deleteCreditCard:(CWVCreditCard*)creditCard {
  [self personalDataManager]->payments_data_manager().DeleteLocalCreditCards(
      {*creditCard.internalCard});
}

@end
