// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_H_
#define BRAVE_IOS_BROWSER_API_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_H_

NS_ASSUME_NONNULL_BEGIN

// From:
// https://source.chromium.org/chromium/chromium/src/+/main:components/omnibox/browser/autocomplete_match_type.h
NS_SWIFT_NAME(AutocompleteMatch.Type)
typedef NS_ENUM(NSUInteger, BraveIOSAutocompleteMatchType) {
  BraveIOSAutocompleteMatchTypeUrlWhatYouTyped = 0,  // The input as a URL.
  BraveIOSAutocompleteMatchTypeHistoryUrl =
      1,  // A past page whose URL contains the input.
  BraveIOSAutocompleteMatchTypeHistoryKeyword =
      4,  // A past page whose keyword contains the input.
  BraveIOSAutocompleteMatchTypeNavSuggest = 5,  // A suggested URL.
  BraveIOSAutocompleteMatchTypeSearchWhatYouTyped =
      6,  // The input as a search query (with the default engine).
  BraveIOSAutocompleteMatchTypeSearchHistory =
      7,  // A past search (with the default engine) containing the input.
  BraveIOSAutocompleteMatchTypeSearchOtherEngine =
      13,  // A search with a non-default engine.
  BraveIOSAutocompleteMatchTypeBookmarkTitle =
      16,  // A bookmark whose title contains the input.
  BraveIOSAutocompleteMatchTypeClipboardUrl =
      19,  // A URL based on the clipboard.
  BraveIOSAutocompleteMatchTypeClipboardText =
      26,  // Text based on the clipboard.
  BraveIOSAutocompleteMatchTypeOpenTab =
      30,  // A URL match amongst the currently open tabs.
};

OBJC_EXPORT
NS_SWIFT_NAME(AutocompleteMatch)
@interface BraveIOSAutocompleteMatch : NSObject
@property(nonatomic, readonly)
    NSString* text;  // The text passed into the classifier
@property(nonatomic, readonly)
    BraveIOSAutocompleteMatchType type;  // The type of match
@property(nonatomic, readonly)
    NSURL* destinationURL;  // The suggested search URL
@end

OBJC_EXPORT
NS_SWIFT_NAME(AutocompleteClassifier)
@interface BraveIOSAutocompleteClassifier : NSObject
+ (nullable BraveIOSAutocompleteMatch*)classify:(NSString*)text;
+ (bool)isSearchType:(BraveIOSAutocompleteMatchType)type;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_H_
