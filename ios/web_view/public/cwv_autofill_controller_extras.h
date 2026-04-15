/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_EXTRAS_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

@class CWVAutofillController;

NS_ASSUME_NONNULL_BEGIN

/// Invoked on the main queue immediately before Chromium shows autofill
/// suggestions (`-[AutofillAgent showAutofillPopup:...]`).
/// `suggestionTypeRawValues` are `autofill::SuggestionType` raw values.
/// `fieldTypeRawOrNSNull` parallels each suggestion: `NSNumber` with
/// `autofill::FieldType` when set, otherwise `NSNull`.
typedef void (^CWVAutofillSuggestionWillPresentHandler)(
    NSUInteger suggestionCount,
    NSArray<NSNumber*>* suggestionTypeRawValues,
    NSArray* fieldTypeRawOrNSNull);

/// Brave extensions for `CWVAutofillController`.
CWV_EXPORT
@interface CWVAutofillController (BraveAutofillExtras)

/// Optional embedder hook to surface suggestion metadata to Swift / UI layers.
@property(nonatomic, copy, nullable)
    CWVAutofillSuggestionWillPresentHandler braveAutofillSuggestionWillPresentHandler;

/// Called from `BraveWebViewAutofillClientIOS` immediately before the standard
/// `showAutofillPopup:suggestionDelegate:` bridge path. Do not call from app
/// code unless you are extending the autofill pipeline.
- (void)brave_dispatchWillPresentSuggestionsWithCount:(NSUInteger)suggestionCount
                                 suggestionTypeRaws:
                                     (NSArray<NSNumber*>*)suggestionTypeRawValues
                                      fieldTypeRaws:(NSArray*)fieldTypeRawOrNSNull;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_EXTRAS_H_
