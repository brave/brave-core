// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class TemplateURLBridge;
@protocol TemplateURLServiceObservation;
@protocol TemplateURLServiceObserverBridge;

/// A backend for keyword/search engines stored, either prepopulated or user
/// added.
///
/// Use the `load` method to trigger a load. When `TemplateURLService` has
/// completed loading, observers are notified via `templateURLServiceChanged`.
NS_SWIFT_NAME(TemplateURLService)
@protocol TemplateURLServiceBridge

/// Whether the initial set of `TemplateURL`s has finished loading. Values
/// fetched before this may be incomplete; observers are notified when loading
/// completes.
@property(readonly, getter=isLoaded) BOOL loaded;

/// Kicks off loading the `TemplateURL`s if not already loading. Safe to call
/// multiple times.
- (void)load;

/// Returns the list of all known `TemplateURL`s that would be shown in the
/// default search provider list (prepopulated and user-added search engines).
@property(readonly, copy) NSArray<TemplateURLBridge*>* templateURLs;

/// Returns the default search provider, or nil if none is set (e.g. the
/// service hasn't loaded yet).
@property(readonly, nullable) TemplateURLBridge* defaultSearchProvider;

/// Returns the default search provider for private browsing. Falls back to
/// `defaultSearchProvider` when no private-specific default has been chosen.
@property(readonly, nullable) TemplateURLBridge* defaultPrivateSearchProvider;

/// Sets the default search provider (by `syncGUID`).
- (void)setUserSelectedDefaultSearchProviderWithGUID:(NSString*)syncGUID;

/// Sets the default search provider (by `syncGUID`) for private browsing.
- (void)setUserSelectedDefaultPrivateSearchProviderWithGUID:(NSString*)syncGUID;

/// Adds a new `TemplateURL` to the model.
///
/// `url` must contain the literal `{searchTerms}` where the query should be
/// substituted.
///
/// This function guarantees that on return the model will not have two
/// `TemplateURL`s with the same keyword. If that means that it cannot add the
/// provided arguments, it will return nil. Otherwise it will return the added
/// `TemplateURL`.
- (nullable TemplateURLBridge*)
    addTemplateURLWithShortName:(NSString*)shortName
                        keyword:(NSString*)keyword
                            url:(NSString*)url
                 suggestionsURL:(nullable NSString*)suggestionsURL
                     faviconURL:(nullable NSURL*)faviconURL;

/// Removes the `TemplateURL` with the given GUID.
- (void)removeTemplateURLWithGUID:(NSString*)syncGUID;

/// Observes changes to the set of `TemplateURL`s. `observer` is held weakly.
/// Retain the returned token for as long as updates are wanted.
- (id<TemplateURLServiceObservation>)addObserver:
    (id<TemplateURLServiceObserverBridge>)observer NS_WARN_UNUSED_RESULT;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_H_
