/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_URL_URL_FORMATTER_H_
#define BRAVE_IOS_BROWSER_API_URL_URL_FORMATTER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger BraveURLSchemeDisplay NS_TYPED_ENUM
    NS_SWIFT_NAME(URLFormatter.SchemeDisplay);

OBJC_EXPORT BraveURLSchemeDisplay const BraveURLSchemeDisplayShow;
OBJC_EXPORT BraveURLSchemeDisplay const BraveURLSchemeDisplayOmitHttpAndHttps;
/// Omit cryptographic (i.e. https and wss).
OBJC_EXPORT BraveURLSchemeDisplay const BraveURLSchemeDisplayOmitCryptographic;

NS_SWIFT_NAME(URLFormatter.FormatType)
typedef NS_OPTIONS(NSUInteger, BraveURLFormatterFormatType) {
  BraveURLFormatterFormatTypeOmitNothing = 0,
  BraveURLFormatterFormatTypeOmitUsernamePassword = 1 << 0,
  BraveURLFormatterFormatTypeOmitHTTP = 1 << 1,
  BraveURLFormatterFormatTypeOmitTrailingSlashOnBareHostname = 1 << 2,
  BraveURLFormatterFormatTypeOmitHTTPS = 1 << 3,
  BraveURLFormatterFormatTypeOmitTrivialSubdomains = 1 << 5,
  BraveURLFormatterFormatTypeTrimAfterHost = 1 << 6,
  BraveURLFormatterFormatTypeOmitFileScheme = 1 << 7,
  BraveURLFormatterFormatTypeOmitMailToScheme = 1 << 8,
  BraveURLFormatterFormatTypeOmitMobilePrefix = 1 << 9,

  /// Omits Username & Password, HTTP (not HTTPS), and Trailing Slash
  BraveURLFormatterFormatTypeOmitDefaults =
      BraveURLFormatterFormatTypeOmitUsernamePassword |
      BraveURLFormatterFormatTypeOmitHTTP |
      BraveURLFormatterFormatTypeOmitTrailingSlashOnBareHostname
};

NS_SWIFT_NAME(URLFormatter.UnescapeRule)
typedef NS_OPTIONS(NSUInteger, BraveURLFormatterUnescapeRule) {
  BraveURLFormatterUnescapeRuleNone = 0,
  BraveURLFormatterUnescapeRuleNormal = 1 << 0,
  BraveURLFormatterUnescapeRuleSpaces = 1 << 1,
  BraveURLFormatterUnescapeRulePathSeparators = 1 << 2,
  BraveURLFormatterUnescapeRuleSpecialCharsExceptPathSeparators = 1 << 3,
  BraveURLFormatterUnescapeRuleReplacePlusWithSpace = 1 << 4
};

OBJC_EXPORT
NS_SWIFT_NAME(URLFormatter)
@interface BraveURLFormatter : NSObject
- (instancetype)init NS_UNAVAILABLE;

/// Format a URL "origin/host" for Security Display
/// origin - The origin of the URL to format
/// schemeDisplay - Determines whether or not to omit the scheme
+ (NSString*)formatURLOriginForSecurityDisplay:(NSString*)origin
                                 schemeDisplay:
                                     (BraveURLSchemeDisplay)schemeDisplay;

/// Format a URL "origin/host" omitting the scheme, path, and trivial
/// sub-domains. origin - The origin to be formatted
+ (NSString*)formatURLOriginForDisplayOmitSchemePathAndTrivialSubdomains:
    (NSString*)origin;

/// Format a URL
/// url - The URL string to be formatted
/// formatTypes - Formatter options when formatting the URL. Typically used to
/// omit certain parts of a URL unescapeOptions - Options passed to the
/// formatter for UN-Escaping parts of a URL
+ (NSString*)formatURL:(NSString*)url
           formatTypes:(BraveURLFormatterFormatType)formatTypes
       unescapeOptions:(BraveURLFormatterUnescapeRule)unescapeOptions;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_URL_FORMATTER_H_
