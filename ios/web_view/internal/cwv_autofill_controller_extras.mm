/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/web_view/public/cwv_autofill_controller_extras.h"

#import <objc/runtime.h>

namespace {

const void* kBraveAutofillSuggestionWillPresentKey =
    &kBraveAutofillSuggestionWillPresentKey;

}  // namespace

@implementation CWVAutofillController (BraveAutofillExtras)

- (void)setBraveAutofillSuggestionWillPresentHandler:
    (CWVAutofillSuggestionWillPresentHandler)handler {
  objc_setAssociatedObject(self, kBraveAutofillSuggestionWillPresentKey, handler,
                           OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (CWVAutofillSuggestionWillPresentHandler)
    braveAutofillSuggestionWillPresentHandler {
  return objc_getAssociatedObject(self, kBraveAutofillSuggestionWillPresentKey);
}

- (void)brave_dispatchWillPresentSuggestionsWithCount:
            (NSUInteger)suggestionCount
                                 suggestionTypeRaws:
                                     (NSArray<NSNumber*>*)suggestionTypeRawValues
                                      fieldTypeRaws:(NSArray*)fieldTypeRawOrNSNull {
  CWVAutofillSuggestionWillPresentHandler handler =
      self.braveAutofillSuggestionWillPresentHandler;
  if (!handler) {
    return;
  }
  if ([NSThread isMainThread]) {
    handler(suggestionCount, suggestionTypeRawValues, fieldTypeRawOrNSNull);
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      handler(suggestionCount, suggestionTypeRawValues, fieldTypeRawOrNSNull);
    });
  }
}

@end
