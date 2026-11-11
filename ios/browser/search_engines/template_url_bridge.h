// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class SearchTermsArgsBridge;
@class SearchTermsDataBridge;
@class TemplateURLRefBridge;

/// IDs of prepopulated (built-in) search engines, for comparison against
/// `TemplateURL.prepopulateID` or for use with
/// `TemplateURLService.templateURL(forPrepopulateID:)`.
typedef NSInteger BravePrepopulatedEngineID NS_TYPED_EXTENSIBLE_ENUM;

OBJC_EXPORT const BravePrepopulatedEngineID BravePrepopulatedEngineIDBrave;

/// A `TemplateURL` represents a single "search engine".
///
/// This object is backed by a `TemplateURL` owned by the `TemplateURLService`
/// and is only valid until the `TemplateURLService` next notifies its observers
/// of a change. Do not hold onto instances across changes, instead re-fetch
/// them from the `TemplateURLService`.
OBJC_EXPORT
NS_SWIFT_NAME(TemplateURL)
@interface TemplateURLBridge : NSObject

/// The GUID that identifies this `TemplateURL` for sync purposes. Stable
/// across launches.
@property(readonly) NSString* syncGUID;

/// The display name of the search engine, e.g. "Brave Search".
@property(readonly) NSString* shortName;

/// The keyword used to trigger the search engine from the omnibox,
/// e.g. "brave.com".
@property(readonly) NSString* keyword;

/// The search URL. Contains placeholders for which callers can substitute
/// values to get a "real" URL; `{searchTerms}` is where the
/// (percent-encoded) query should be substituted.
@property(readonly) NSString* url;

/// The suggestions URL template, if any. Contains `{searchTerms}`.
@property(readonly, nullable) NSString* suggestionsURL;

/// The URL of the search engine's favicon, if any.
@property(readonly, nullable) NSURL* faviconURL;

/// The ID of the prepopulated engine this `TemplateURL` was created from, or
/// `0` if this is not a prepopulated engine.
@property(readonly) BravePrepopulatedEngineID prepopulateID;

/// Whether this is a prepopulated (built-in) search engine as opposed to a
/// user-added one.
@property(readonly, getter=isPrepopulated) BOOL prepopulated;

/// The `TemplateURLRef` for the search URL.
@property(readonly) TemplateURLRefBridge* urlRef;

/// Uses the alternate URLs and the search URL to match the provided `url` and
/// extract the search terms from it. Returns nil if no search terms can be
/// matched.
- (nullable NSString*)extractSearchTermsFromURL:(NSURL*)url
                                searchTermsData:
                                    (SearchTermsDataBridge*)searchTermsData
    NS_SWIFT_NAME(extractSearchTerms(from:searchTermsData:));

/// Generates a favicon URL from the specified URL.
+ (nullable NSURL*)generateFaviconURL:(NSURL*)url
    NS_SWIFT_NAME(generateFaviconURL(_:));

/// Generates a suitable keyword for the specified URL.
+ (NSString*)generateKeyword:(NSURL*)url NS_SWIFT_NAME(generateKeyword(_:));

- (instancetype)init NS_UNAVAILABLE;

@end

/// A `TemplateURLRef` represents a single URL within the larger `TemplateURL`
/// (e.g. the search URL), and provides the means to substitute search terms
/// into it.
///
/// This object is backed by the `TemplateURL` it was obtained from and has the
/// same lifetime requirements as `TemplateURL`.
OBJC_EXPORT
NS_SWIFT_NAME(TemplateURLRef)
@interface TemplateURLRefBridge : NSObject

/// Returns a URL that is the result of substituting the search terms and
/// other parameters into the URL. Returns nil if the resulting URL is
/// invalid.
- (nullable NSURL*)replaceSearchTerms:(SearchTermsArgsBridge*)searchTermsArgs
                      searchTermsData:(SearchTermsDataBridge*)searchTermsData
    NS_SWIFT_NAME(replaceSearchTerms(_:searchTermsData:));

- (instancetype)init NS_UNAVAILABLE;

@end

/// Arguments passed to `TemplateURLRef.replaceSearchTerms(_:searchTermsData:)`.
OBJC_EXPORT
NS_SWIFT_NAME(TemplateURLRef.SearchTermsArgs)
@interface SearchTermsArgsBridge : NSObject

/// The search terms (query) to substitute into the URL.
@property(nonatomic, copy) NSString* searchTerms;

- (instancetype)initWithSearchTerms:(NSString*)searchTerms
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

/// Provides the data used to substitute placeholders in a `TemplateURLRef`.
/// Obtain an instance from `TemplateURLService.searchTermsData`.
OBJC_EXPORT
NS_SWIFT_NAME(SearchTermsData)
@interface SearchTermsDataBridge : NSObject
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_H_
