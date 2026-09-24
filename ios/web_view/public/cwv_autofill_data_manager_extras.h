// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_DATA_MANAGER_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_DATA_MANAGER_EXTRAS_H_

#import <Foundation/Foundation.h>

#include "cwv_autofill_data_manager.h"  // NOLINT

@class CWVCreditCard;

NS_ASSUME_NONNULL_BEGIN

/// Adds additional functionality to add/modify/delete credit cards
@interface CWVAutofillDataManager (Extras)

/// Add a local credit card to the database
- (void)addCreditCard:(CWVCreditCard*)creditCard;

/// Update an existing local credit card in the database
- (void)updateCreditCard:(CWVCreditCard*)creditCard;

/// Delete an existing local credit card from the database
- (void)deleteCreditCard:(CWVCreditCard*)creditCard;

/// Add a new local profile/address to the database
- (void)addProfile:(CWVAutofillProfile*)profile;

/// Creates a new CWVAutofillProfile pre-filled with the users current country
/// code as the default
- (CWVAutofillProfile*)defaultAutofillProfileForNewAddress;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_DATA_MANAGER_EXTRAS_H_
