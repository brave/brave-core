/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_URL_URL_PATTERN_IOS_H_
#define BRAVE_IOS_BROWSER_API_URL_URL_PATTERN_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSInteger, URLPatternIOSScheme) {
  URLPatternIOSSchemeNone = 0,
  URLPatternIOSSchemeHttp = 1 << 0,
  URLPatternIOSSchemeHttps = 1 << 1,
  URLPatternIOSSchemeFile = 1 << 2,
  URLPatternIOSSchemeFtp = 1 << 3,
  URLPatternIOSSchemeChromeUI = 1 << 4,
  URLPatternIOSSchemeExtension = 1 << 5,
  URLPatternIOSSchemeFilesystem = 1 << 6,
  URLPatternIOSSchemeWs = 1 << 7,
  URLPatternIOSSchemeWss = 1 << 8,
  URLPatternIOSSchemeData = 1 << 9,
  URLPatternIOSSchemeUUIDInPackage = 1 << 10,
  URLPatternIOSSchemeAll = -1
} NS_SWIFT_NAME(URLPattern.Scheme);

typedef NSInteger URLPatternIOSParseResult NS_TYPED_ENUM
    NS_SWIFT_NAME(URLPattern.ParseResult);
OBJC_EXPORT URLPatternIOSParseResult const URLPatternIOSParseResultSuccess;
OBJC_EXPORT URLPatternIOSParseResult const
    URLPatternIOSParseResultMissingSchemeSeparator;
OBJC_EXPORT URLPatternIOSParseResult const
    URLPatternIOSParseResultInvalidScheme;
OBJC_EXPORT URLPatternIOSParseResult const
    URLPatternIOSParseResultWrongSchemeSeparator;
OBJC_EXPORT URLPatternIOSParseResult const URLPatternIOSParseResultEmptyHost;
OBJC_EXPORT URLPatternIOSParseResult const
    URLPatternIOSParseResultInvalidHostWildcard;
OBJC_EXPORT URLPatternIOSParseResult const URLPatternIOSParseResultEmptyPath;
OBJC_EXPORT URLPatternIOSParseResult const URLPatternIOSParseResultInvalidPort;
OBJC_EXPORT URLPatternIOSParseResult const URLPatternIOSParseResultInvalidHost;

/// A wrapper around Chromium's `URLPattern` functionality
OBJC_EXPORT
NS_SWIFT_NAME(URLPattern)
@interface URLPatternIOS : NSObject <NSCopying>

/// A pattern that will match all urls.
///
/// Can be passed into `parsePattern` or `initWithValidSchemes:patternLiteral:`
@property(class, readonly) NSString* allURLsPattern;

@property(nonatomic) URLPatternIOSScheme validSchemes;
@property(nonatomic) NSString* scheme;
@property(nonatomic) NSString* host;
@property(nonatomic) NSString* port;
@property(nonatomic) NSString* path;
@property(nonatomic) bool isMatchingSubdomains;
@property(nonatomic) bool isMatchingAllURLs;

/// Convenience to construct an empty URLPattern with no schemes setup
- (instancetype)init;

- (instancetype)initWithValidSchemes:(URLPatternIOSScheme)schemes
    NS_DESIGNATED_INITIALIZER;

/// Convenience to construct a URLPattern from a string literal. If the string
/// is not known ahead of time, use `parsePattern:` instead
- (instancetype)initWithValidSchemes:(URLPatternIOSScheme)schemes
                      patternLiteral:(NSString*)patternLiteral;

/// Sets the current pattern to match against
- (URLPatternIOSParseResult)parsePattern:(NSString*)pattern
    NS_SWIFT_NAME(parse(pattern:));

/// Returns true if the specified scheme can be used in this URL pattern, and
/// false otherwise.
- (bool)isValidScheme:(NSString*)scheme;

/// Returns true if this instance matches the specified URL. Always returns
/// false for invalid URLs.
- (bool)matchesURL:(NSURL*)url;

/// Returns true if this instance matches the specified security origin.
- (bool)matchesSecurityOrigin:(NSURL*)origin;

/// Returns true if `scheme` matches our scheme.
/// Note that if scheme is "filesystem", this may fail whereas `matchesURL`
/// may succeed.  `matchesURL` is smart enough to look at the inner url instead
/// of the outer "filesystem:" part.
- (bool)matchesScheme:(NSString*)scheme;

/// Returns true if `host` matches our host.
- (bool)matchesHost:(NSString*)host;

/// Returns true if `path` matches our path.
- (bool)matchesPath:(NSString*)path;

/// Returns true if the pattern only matches a single origin. The pattern may
/// include a path.
- (bool)matchesSingleOrigin;

/// Returns true if this pattern matches all possible URLs that `pattern` can
/// match. For example, http://*.google.com encompasses http://www.google.com.
- (bool)containsOtherURLPattern:(URLPatternIOS*)pattern;

/// Determines whether there is a URL that would match this instance and
/// another instance.
- (bool)overlapsWithOtherURLPattern:(URLPatternIOS*)pattern
    NS_SWIFT_NAME(overlapsWithOtherURLPattern(_:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_URL_PATTERN_IOS_H_
