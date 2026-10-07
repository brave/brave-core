// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_H_

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include "brave/ios/browser/search_engines/template_url_bridge.h"
#else
#include "template_url_bridge.h"  // NOLINT
#endif

NS_ASSUME_NONNULL_BEGIN

@protocol TemplateURLServiceScopedObservation;
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

/// Returns the list of all known `TemplateURL`s (prepopulated and user-added
/// search engines). Starter pack engines are excluded as they are not
/// supported on iOS.
@property(readonly, copy) NSArray<TemplateURLBridge*>* templateURLs;

/// Whether `templateURL` should be shown in the list of engines most likely
/// to be selected as the default search provider (i.e. it is the current
/// default or is prepopulated).
- (BOOL)showInDefaultList:(TemplateURLBridge*)templateURL
    NS_SWIFT_NAME(showInDefaultList(_:));

/// Returns the `TemplateURL` with the given GUID, or nil if none exists.
- (nullable TemplateURLBridge*)templateURLForGUID:(NSString*)syncGUID
    NS_SWIFT_NAME(templateURL(forGUID:));

/// Returns the prepopulated `TemplateURL` with the given ID, or nil if none
/// exists.
- (nullable TemplateURLBridge*)templateURLForPrepopulateID:
    (BravePrepopulatedEngineID)prepopulateID
    NS_SWIFT_NAME(templateURL(forPrepopulateID:));

/// Returns the default search provider, or nil if none is set (e.g. the
/// service hasn't loaded yet).
@property(readonly, nullable) TemplateURLBridge* defaultSearchProvider;

/// Returns the default search provider for private browsing. Falls back to
/// `defaultSearchProvider` when no private-specific default has been chosen.
@property(readonly, nullable) TemplateURLBridge* defaultPrivateSearchProvider;

/// Generates a search results page URL for the default search provider with
/// the given search terms. Returns nil if the default search provider is not
/// available.
- (nullable NSURL*)generateSearchURLForDefaultSearchProvider:
    (NSString*)searchTerms
    NS_SWIFT_NAME(generateSearchURLForDefaultSearchProvider(_:));

/// The data used to substitute placeholders in a `TemplateURLRef`.
@property(readonly) SearchTermsDataBridge* searchTermsData;

/// Sets the default search provider (by `syncGUID`).
- (void)setUserSelectedDefaultSearchProviderWithGUID:(NSString*)syncGUID
    NS_SWIFT_NAME(setUserSelectedDefaultSearchProvider(withGUID:));

/// Sets the default search provider (by `syncGUID`) for private browsing.
- (void)setUserSelectedDefaultPrivateSearchProviderWithGUID:(NSString*)syncGUID
    NS_SWIFT_NAME(setUserSelectedDefaultPrivateSearchProvider(withGUID:));

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
                     faviconURL:(nullable NSURL*)faviconURL
    NS_SWIFT_NAME(addTemplateURL(shortName:keyword:url:suggestionsURL:faviconURL:));  // NOLINT

/// Resets the title, keyword and search url of the `TemplateURL` with the
/// given GUID. The `TemplateURL` is marked as not replaceable.
///
/// `searchURL` must contain the literal `{searchTerms}` where the query should
/// be substituted.
- (void)resetTemplateURLWithGUID:(NSString*)syncGUID
                           title:(NSString*)title
                         keyword:(NSString*)keyword
                       searchURL:(NSString*)searchURL
    NS_SWIFT_NAME(resetTemplateURL(withGUID:title:keyword:searchURL:));

/// Removes the `TemplateURL` with the given GUID.
- (void)removeTemplateURLWithGUID:(NSString*)syncGUID
    NS_SWIFT_NAME(removeTemplateURL(withGUID:));

/// Observes changes to the set of `TemplateURL`s. `observer` is held weakly.
/// Retain the returned token for as long as updates are wanted.
- (id<TemplateURLServiceScopedObservation>)addObserver:
    (id<TemplateURLServiceObserverBridge>)observer NS_WARN_UNUSED_RESULT;

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
@protocol TemplateURLServiceScopedObservation

/// Stops the observation early. Also happens automatically when this object
/// is deallocated.
- (void)invalidate;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_H_
