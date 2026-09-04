// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// A `TemplateURL` represents a single "search engine".
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

/// Whether this is a prepopulated (built-in) search engine as opposed to a
/// user-added one.
@property(readonly, getter=isPrepopulated) BOOL prepopulated;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_SWIFT_NAME(TemplateURLServiceObserver)
@protocol TemplateURLServiceObserverBridge

/// Notification that the template url model has changed in some way. Also
/// fired when the service completes loading the initial set of
/// `TemplateURL`s.
- (void)templateURLServiceChanged;

@end

/// A token representing one active observation of a
/// `TemplateURLServiceBridge`. Retain this for as long as updates are wanted;
/// releasing it (or calling `invalidate`) stops the observation.
NS_SWIFT_NAME(TemplateURLServiceObservation)
@protocol TemplateURLServiceObservation

/// Stops the observation early. Also happens automatically when this object
/// is deallocated.
- (void)invalidate;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_H_
