// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_LOCALIZED_TEXTS_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_LOCALIZED_TEXTS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// The element picker's button labels. iOS localizes in the Swift layer, so
// these are supplied by the bridge rather than read from `l10n_util`.
OBJC_EXPORT
@interface ElementPickerLocalizedTexts : NSObject

@property(nonatomic, copy) NSString* createDisabledText;
@property(nonatomic, copy) NSString* createEnabledText;
@property(nonatomic, copy) NSString* manageText;
@property(nonatomic, copy) NSString* showRulesText;
@property(nonatomic, copy) NSString* hideRulesText;
@property(nonatomic, copy) NSString* quitText;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (instancetype)initWithCreateDisabledText:(NSString*)createDisabledText
                         createEnabledText:(NSString*)createEnabledText
                                manageText:(NSString*)manageText
                             showRulesText:(NSString*)showRulesText
                             hideRulesText:(NSString*)hideRulesText
                                  quitText:(NSString*)quitText;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_LOCALIZED_TEXTS_H_
