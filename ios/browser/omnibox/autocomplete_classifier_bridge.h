// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_BRIDGE_H_

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include "brave/ios/browser/omnibox/autocomplete_match_bridge.h"
#else
#include "autocomplete_match_bridge.h"  // NOLINT
#endif

NS_ASSUME_NONNULL_BEGIN

/// Namespace for omnibox event related types.
OBJC_EXPORT
NS_SWIFT_NAME(OmniboxEventProto)
@interface OmniboxEventProtoBridge : NSObject
- (instancetype)init NS_UNAVAILABLE;
@end

/// The type of page the user was on when they used the omnibox.
typedef NSInteger OmniboxEventProtoPageClassification
    NS_TYPED_ENUM NS_SWIFT_NAME(OmniboxEventProto.PageClassification);

/// An invalid URL; shouldn't happen.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationInvalidSpec;
/// The new tab page.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationNTP;
/// about:blank.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationBlank;
/// The user's home page. Note that if the home page is set to any of the new
/// tab page versions or to about:blank, then we'll classify the page into
/// those categories, not `homePage`.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationHomePage;
/// The catch-all entry of everything not included somewhere else on this
/// list.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationOther;
/// The user is on a search result page that does search term replacement.
/// This means the search terms are shown in the omnibox instead of the URL.
/// In other words: Query in Omnibox is Active for this SRP.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationSearchResultPageDoingSearchTermReplacement;
/// The new tab page in which this omnibox interaction first started with the
/// user having focus in the omnibox.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationInstantNTPWithOmniboxAsStartingFocus;
/// The user is on a search result page that does not do search term
/// replacement. This means the URL of the SRP is shown in the omnibox.
/// In other words: Query in Omnibox is Inactive for this SRP.
OBJC_EXPORT OmniboxEventProtoPageClassification const
    OmniboxEventProtoPageClassificationSearchResultPageNoSearchTermReplacement;

/// The result of `AutocompleteClassifier.classify`.
OBJC_EXPORT
NS_SWIFT_NAME(AutocompleteClassifierResult)
@interface AutocompleteClassifierResultBridge : NSObject

/// The default match for the classified input.
@property(readonly) AutocompleteMatchBridge* match;

/// The navigational URL (if any) in case of an accidental search.
@property(readonly, copy, nullable) NSURL* alternateNavURL;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithMatch:(AutocompleteMatchBridge*)match
              alternateNavURL:(nullable NSURL*)alternateNavURL
    NS_DESIGNATED_INITIALIZER;

@end

NS_SWIFT_NAME(AutocompleteClassifier)
NS_SWIFT_UI_ACTOR
@protocol AutocompleteClassifierBridge

/// Given some string `text` that the user wants to use for navigation,
/// determines how it should be interpreted.
///
/// `inKeywordMode` should be true the when keyword UI is onscreen.
/// `allowExactKeywordMatch` should be true when treating the string as a
/// potential keyword search is valid.
/// `pageClassification` gives information about the context (e.g., is the
/// user on a search results page doing search term replacement); this may be
/// useful in deciding how the input should be interpreted.
///
/// Returns the default match for this input, if any, along with the
/// navigational URL (if any) in case of an accidental search. For invalid
/// input, there will be no default match and `nil` is returned.
- (nullable AutocompleteClassifierResultBridge*)
                  classify:(NSString*)text
             inKeywordMode:(BOOL)inKeywordMode
    allowExactKeywordMatch:(BOOL)allowExactKeywordMatch
        pageClassification:
            (OmniboxEventProtoPageClassification)pageClassification
    NS_SWIFT_NAME(classify(_:inKeywordMode:allowExactKeywordMatch:pageClassification:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_BRIDGE_H_
