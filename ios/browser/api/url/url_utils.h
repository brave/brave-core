/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_URL_URL_UTILS_H_
#define BRAVE_IOS_BROWSER_API_URL_URL_UTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NSURL (Utilities)

/// Returns the eTLD+1 including private registries.
@property(readonly, nullable)
    NSString* brave_domainAndRegistry NS_SWIFT_NAME(domainAndRegistry);

/// Returns the eTLD+1 not including private registries.
@property(readonly, nullable)
    NSString* brave_domainAndRegistryExcludingPrivateRegistries NS_SWIFT_NAME(
        domainAndRegistryExcludingPrivateRegistries);

/// Returns `true` when the `host` is an IP address.
@property(readonly) bool brave_isHostIPAddress NS_SWIFT_NAME(isHostIPAddress);

/// The string representation of a URL is called the spec.
/// Returns the canonicalized URL's spec.
@property(readonly) NSString* brave_spec NS_SWIFT_NAME(spec);

/// Appends the key=value query parameter, escaping if needed.
- (NSURL*)brave_addingQueryParameter:(NSString*)key
                               value:(NSString*)value
    NS_SWIFT_NAME(addingQueryParameter(key:value:));

/// Appends OR Replaces the key=value query parameter, escaping if needed.
- (NSURL*)brave_replacingQueryParameter:(NSString*)key
                                  value:(NSString*)value
    NS_SWIFT_NAME(replacingQueryParameter(key:value:));

/// Returns the value for the query parameter. NULL if no parameter exists.
- (nullable NSString*)brave_valueForQueryParameter:(NSString*)key
    NS_SWIFT_NAME(valueForQueryParameter(key:));

/// Returns true if the URL has a scheme.
- (bool)brave_hasScheme:(NSString*)scheme NS_SWIFT_NAME(hasScheme(scheme:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_URL_UTILS_H_
