// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/autofill/autofill_credit_card_util.h"

#include "base/strings/sys_string_conversions.h"
#include "components/application_locale_storage/application_locale_storage.h"
#include "components/autofill/core/browser/data_model/payments/credit_card.h"
#include "ios/chrome/browser/autofill/ui_bundled/autofill_credit_card_util.h"
#include "ios/web_view/internal/autofill/cwv_credit_card_internal.h"
#include "ios/web_view/public/cwv_credit_card.h"

@implementation CWVCreditCard (Util)

+ (CWVCreditCard*)creditCardWithHolderName:(NSString*)cardHolderName
                                cardNumber:(NSString*)cardNumber
                           expirationMonth:(NSString*)expirationMonth
                            expirationYear:(NSString*)expirationYear
                              cardNickname:(NSString*)cardNickname
                                   cardCvc:(NSString*)cardCvc
                                 appLocale:(NSString*)appLocale {
  autofill::CreditCard card = [AutofillCreditCardUtil
      creditCardWithHolderName:cardHolderName
                    cardNumber:cardNumber
               expirationMonth:expirationMonth
                expirationYear:expirationYear
                  cardNickname:cardNickname
                       cardCvc:cardCvc
                      appLocal:base::SysNSStringToUTF8(appLocale)];
  return [[CWVCreditCard alloc] initWithCreditCard:card];
}

+ (CWVCreditCard*)updatedCreditCard:(CWVCreditCard*)creditCard
                     cardHolderName:(NSString*)cardHolderName
                         cardNumber:(NSString*)cardNumber
                    expirationMonth:(NSString*)expirationMonth
                     expirationYear:(NSString*)expirationYear
                       cardNickname:(NSString*)cardNickname
                            cardCvc:(NSString*)cardCvc
                          appLocale:(NSString*)appLocale {
  autofill::CreditCard* internalCard = [creditCard internalCard];
  [AutofillCreditCardUtil updateCreditCard:internalCard
                            cardHolderName:cardHolderName
                                cardNumber:cardNumber
                           expirationMonth:expirationMonth
                            expirationYear:expirationYear
                              cardNickname:cardNickname
                                   cardCvc:cardCvc
                                  appLocal:base::SysNSStringToUTF8(appLocale)];
  return [[CWVCreditCard alloc] initWithCreditCard:*internalCard];
}

+ (BOOL)isValidCreditCard:(NSString*)cardNumber
          expirationMonth:(NSString*)expirationMonth
           expirationYear:(NSString*)expirationYear
             cardNickname:(NSString*)cardNickname
                  cardCvc:(NSString*)cardCvc
                appLocale:(NSString*)appLocale {
  return [AutofillCreditCardUtil
      isValidCreditCard:cardNumber
        expirationMonth:expirationMonth
         expirationYear:expirationYear
           cardNickname:cardNickname
                cardCvc:cardCvc
               appLocal:base::SysNSStringToUTF8(appLocale)];
}

+ (BOOL)isValidCreditCardNumber:(NSString*)cardNumber
                      appLocale:(NSString*)appLocale {
  return [AutofillCreditCardUtil
      isValidCreditCardNumber:cardNumber
                     appLocal:base::SysNSStringToUTF8(appLocale)];
}

+ (BOOL)isValidCreditCardExpirationMonth:(NSString*)expirationMonth {
  return
      [AutofillCreditCardUtil isValidCreditCardExpirationMonth:expirationMonth];
}

+ (BOOL)isValidCreditCardExpirationYear:(NSString*)expirationYear
                              appLocale:(NSString*)appLocale {
  return [AutofillCreditCardUtil
      isValidCreditCardExpirationYear:expirationYear
                             appLocal:base::SysNSStringToUTF8(appLocale)];
}

+ (BOOL)isValidCardNickname:(NSString*)cardNickname {
  return [AutofillCreditCardUtil isValidCardNickname:cardNickname];
}

+ (BOOL)isValidCardCvc:(NSString*)cardCvc {
  return [AutofillCreditCardUtil isValidCardCvc:cardCvc];
}

@end
