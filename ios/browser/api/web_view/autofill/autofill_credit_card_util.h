// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_AUTOFILL_CREDIT_CARD_UTIL_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_AUTOFILL_CREDIT_CARD_UTIL_H_

#import <Foundation/Foundation.h>

#include "cwv_credit_card.h"  // NOLINT

NS_ASSUME_NONNULL_BEGIN

// A set of methods for using Chromium's `AutofillCreditCardUtil` but with
// Chromium web view's `CWVCreditCard` wrapper
@interface CWVCreditCard (Util)

+ (CWVCreditCard*)creditCardWithHolderName:(NSString*)cardHolderName
                                cardNumber:(NSString*)cardNumber
                           expirationMonth:(NSString*)expirationMonth
                            expirationYear:(NSString*)expirationYear
                              cardNickname:(NSString*)cardNickname
                                   cardCvc:(NSString*)cardCvc
                                 appLocale:(NSString*)appLocale;

+ (CWVCreditCard*)updatedCreditCard:(CWVCreditCard*)creditCard
                     cardHolderName:(NSString*)cardHolderName
                         cardNumber:(NSString*)cardNumber
                    expirationMonth:(NSString*)expirationMonth
                     expirationYear:(NSString*)expirationYear
                       cardNickname:(NSString*)cardNickname
                            cardCvc:(NSString*)cardCvc
                          appLocale:(NSString*)appLocale;

+ (BOOL)isValidCreditCard:(NSString*)cardNumber
          expirationMonth:(NSString*)expirationMonth
           expirationYear:(NSString*)expirationYear
             cardNickname:(NSString*)cardNickname
                  cardCvc:(NSString*)cardCvc
                appLocale:(NSString*)appLocale;

+ (BOOL)isValidCreditCardNumber:(NSString*)cardNumber
                      appLocale:(NSString*)appLocale;

+ (BOOL)isValidCreditCardExpirationMonth:(NSString*)expirationMonth;

+ (BOOL)isValidCreditCardExpirationYear:(NSString*)expirationYear
                              appLocale:(NSString*)appLocale;

+ (BOOL)isValidCardNickname:(NSString*)cardNickname;

+ (BOOL)isValidCardCvc:(NSString*)cardCvc;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_AUTOFILL_CREDIT_CARD_UTIL_H_
