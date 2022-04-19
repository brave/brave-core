/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_URL_URL_ORIGIN_IOS_H_
#define BRAVE_IOS_BROWSER_API_URL_URL_ORIGIN_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// A wrapper around Chromium's `url::Origin`
///
/// For more info, see
/// https://source.chromium.org/chromium/chromium/src/+/main:url/origin.h
OBJC_EXPORT
NS_SWIFT_NAME(URLOrigin)
@interface URLOriginIOS : NSObject <NSCopying>

/// Creates an opaque Origin with a nonce that is different from all previously
/// existing origins.
- (instancetype)init;

/// Creates an Origin from |url|, as described at
/// https://url.spec.whatwg.org/#origin, with the following additions:
///
/// 1. If |url| is invalid or non-standard, an opaque Origin is constructed.
/// 2. 'filesystem' URLs behave as 'blob' URLs (that is, the origin is parsed
///    out of everything in the URL which follows the scheme).
/// 3. 'file' URLs all parse as ("file", "", 0).
///
/// Note that the returned Origin may have a different scheme and host from
/// |url| (e.g. in case of blob URLs - see OriginTest.ConstructFromGURL).
- (instancetype)initWithURL:(NSURL*)url;

/// Creates an Origin from a |scheme|, |host|, and |port|. All the parameters
/// must be valid and canonicalized. Returns nullopt if any parameter is not
/// canonical, or if all the parameters are empty.
- (nullable instancetype)initWithScheme:(NSString*)scheme
                                   host:(NSString*)host
                                   port:(uint16_t)port;

/// Creates an Origin for the resource |url| as if it were requested
/// from the context of |base_origin|.  If |url| is standard
/// (in the sense that it embeds a complete origin, like http/https),
/// this returns the same value as would Create().
///
/// If |url| is "about:blank", this returns a copy of |base_origin|.
///
/// Otherwise, returns a new opaque origin derived from |base_origin|.
/// In this case, the resulting opaque origin will inherit the tuple
/// (or precursor tuple) of |base_origin|, but will not be same origin
/// with |base_origin|, even if |base_origin| is already opaque.
+ (instancetype)originResolvingURL:(NSURL*)url
                        baseOrigin:(URLOriginIOS*)origin
    NS_SWIFT_NAME(init(resolvingURL:baseOrigin:));

/// Whether or not this origin is opaque
@property(readonly) bool isOpaque;

/// The scheme or an empty string if opaque
@property(readonly) NSString* scheme;

/// The host or an empty string if opaque
@property(readonly) NSString* host;

/// The port or 0 if opaque
@property(readonly) uint16_t port;

/// An ASCII serialization of the Origin as per Section 6.2 of RFC 6454, with
/// the addition that all Origins with a 'file' scheme serialize to "file://".
///
/// Returns nil if opaque
@property(readonly, nullable) NSString* serialized;

/// Efficiently returns what GURL(Serialize()) would without re-parsing the
/// URL. This can be used for the (rare) times a GURL representation is needed
/// for an Origin.
/// Note: The returned URL will not necessarily be serialized to the same value
/// as the Origin would. The GURL will have an added "/" path for Origins with
/// valid SchemeHostPorts and file Origins.
///
/// Try not to use this method under normal circumstances, as it loses type
/// information. Downstream consumers can mistake the returned GURL with a full
/// URL (e.g. with a path component).
///
/// Returns nil if opaque
@property(readonly, nullable) NSURL* url;

/// Non-opaque origin is "same-origin" with `url` if their schemes, hosts, and
/// ports are exact matches.  Opaque origin is never "same-origin" with any
/// `url`.  about:blank, about:srcdoc, and invalid GURLs are never
/// "same-origin" with any origin.  This method is a shorthand for
/// `origin.IsSameOriginWith(url::Origin::Create(url))`.
///
/// See also CanBeDerivedFrom.
- (bool)isSameOriginWithURL:(NSURL*)url;

/// This method returns true for any |url| which if navigated to could result
/// in an origin compatible with |this|.
- (bool)canBeDerivedFromURL:(NSURL*)url;

/// Same as GURL::DomainIs. If |this| origin is opaque, then returns false.
- (bool)isCanonicalDomain:(NSString*)canonicalDomain;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_URL_ORIGIN_IOS_H_
