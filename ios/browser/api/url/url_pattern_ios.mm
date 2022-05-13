/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/url/url_pattern_ios.h"

#include "base/check_op.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "extensions/common/url_pattern.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

URLPatternIOSScheme IOSSchemeForSchemeMask(
    int /* URLPattern::SchemeMasks */ masks) {
  if (masks == -1) {
    return URLPatternIOSSchemeAll;
  }
  URLPatternIOSScheme scheme = URLPatternIOSSchemeNone;
  if (masks & URLPattern::SchemeMasks::SCHEME_HTTP) {
    scheme |= URLPatternIOSSchemeHttp;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_HTTPS) {
    scheme |= URLPatternIOSSchemeHttps;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_FILE) {
    scheme |= URLPatternIOSSchemeFile;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_FTP) {
    scheme |= URLPatternIOSSchemeFtp;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_CHROMEUI) {
    scheme |= URLPatternIOSSchemeChromeUI;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_EXTENSION) {
    scheme |= URLPatternIOSSchemeExtension;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_FILESYSTEM) {
    scheme |= URLPatternIOSSchemeFilesystem;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_WS) {
    scheme |= URLPatternIOSSchemeWs;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_WSS) {
    scheme |= URLPatternIOSSchemeWss;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_DATA) {
    scheme |= URLPatternIOSSchemeData;
  }
  if (masks & URLPattern::SchemeMasks::SCHEME_UUID_IN_PACKAGE) {
    scheme |= URLPatternIOSSchemeUUIDInPackage;
  }
  DCHECK_EQ(static_cast<int>(scheme), masks)
      << "Obj-C mask conversion mismatch. Got " << static_cast<int>(scheme)
      << " Expected: " << masks;
  return scheme;
}

int /* URLPattern::SchemeMasks */ SchemeMasksForIOSScheme(
    URLPatternIOSScheme masks) {
  if (masks == URLPatternIOSSchemeAll) {
    return URLPattern::SchemeMasks::SCHEME_ALL;
  }
  int scheme = URLPattern::SchemeMasks::SCHEME_NONE;
  if (masks & URLPatternIOSSchemeHttp) {
    scheme |= URLPattern::SchemeMasks::SCHEME_HTTP;
  }
  if (masks & URLPatternIOSSchemeHttps) {
    scheme |= URLPattern::SchemeMasks::SCHEME_HTTPS;
  }
  if (masks & URLPatternIOSSchemeFile) {
    scheme |= URLPattern::SchemeMasks::SCHEME_FILE;
  }
  if (masks & URLPatternIOSSchemeFtp) {
    scheme |= URLPattern::SchemeMasks::SCHEME_FTP;
  }
  if (masks & URLPatternIOSSchemeChromeUI) {
    scheme |= URLPattern::SchemeMasks::SCHEME_CHROMEUI;
  }
  if (masks & URLPatternIOSSchemeExtension) {
    scheme |= URLPattern::SchemeMasks::SCHEME_EXTENSION;
  }
  if (masks & URLPatternIOSSchemeFilesystem) {
    scheme |= URLPattern::SchemeMasks::SCHEME_FILESYSTEM;
  }
  if (masks & URLPatternIOSSchemeWs) {
    scheme |= URLPattern::SchemeMasks::SCHEME_WS;
  }
  if (masks & URLPatternIOSSchemeWss) {
    scheme |= URLPattern::SchemeMasks::SCHEME_WSS;
  }
  if (masks & URLPatternIOSSchemeData) {
    scheme |= URLPattern::SchemeMasks::SCHEME_DATA;
  }
  if (masks & URLPatternIOSSchemeUUIDInPackage) {
    scheme |= URLPattern::SchemeMasks::SCHEME_UUID_IN_PACKAGE;
  }
  DCHECK_EQ(static_cast<int>(masks), scheme)
      << "Obj-C mask conversion mismatch. Got " << scheme
      << " Expected: " << static_cast<int>(masks);
  return scheme;
}

URLPatternIOSParseResult const URLPatternIOSParseResultSuccess =
    static_cast<NSInteger>(URLPattern::ParseResult::kSuccess);
URLPatternIOSParseResult const URLPatternIOSParseResultMissingSchemeSeparator =
    static_cast<NSInteger>(URLPattern::ParseResult::kMissingSchemeSeparator);
URLPatternIOSParseResult const URLPatternIOSParseResultInvalidScheme =
    static_cast<NSInteger>(URLPattern::ParseResult::kInvalidScheme);
URLPatternIOSParseResult const URLPatternIOSParseResultWrongSchemeSeparator =
    static_cast<NSInteger>(URLPattern::ParseResult::kWrongSchemeSeparator);
URLPatternIOSParseResult const URLPatternIOSParseResultEmptyHost =
    static_cast<NSInteger>(URLPattern::ParseResult::kEmptyHost);
URLPatternIOSParseResult const URLPatternIOSParseResultInvalidHostWildcard =
    static_cast<NSInteger>(URLPattern::ParseResult::kInvalidHostWildcard);
URLPatternIOSParseResult const URLPatternIOSParseResultEmptyPath =
    static_cast<NSInteger>(URLPattern::ParseResult::kEmptyPath);
URLPatternIOSParseResult const URLPatternIOSParseResultInvalidPort =
    static_cast<NSInteger>(URLPattern::ParseResult::kInvalidPort);
URLPatternIOSParseResult const URLPatternIOSParseResultInvalidHost =
    static_cast<NSInteger>(URLPattern::ParseResult::kInvalidHost);

@implementation URLPatternIOS {
  std::unique_ptr<URLPattern> url_pattern;
}

+ (NSString*)allURLsPattern {
  return base::SysUTF8ToNSString(URLPattern::kAllUrlsPattern);
}

- (instancetype)init {
  return [self initWithValidSchemes:URLPatternIOSSchemeNone];
}

- (instancetype)initWithValidSchemes:(URLPatternIOSScheme)schemes {
  if ((self = [super init])) {
    url_pattern =
        std::make_unique<URLPattern>(SchemeMasksForIOSScheme(schemes));
  }
  return self;
}

- (instancetype)initWithValidSchemes:(URLPatternIOSScheme)schemes
                      patternLiteral:(NSString*)patternLiteral {
  if ((self = [self initWithValidSchemes:schemes])) {
    const auto parseResult = [self parsePattern:patternLiteral];
    if (parseResult != URLPatternIOSParseResultSuccess) {
      VLOG(0) << "Failed to parse pattern literal: \""
              << base::SysNSStringToUTF8(patternLiteral) << "\". Reason: "
              << URLPattern::GetParseResultString(
                     static_cast<URLPattern::ParseResult>(parseResult));
    }
  }
  return self;
}

- (bool)isEqual:(id)obj {
  if ([obj isKindOfClass:URLPatternIOS.class]) {
    return *url_pattern == *static_cast<URLPatternIOS*>(obj)->url_pattern;
  }
  return [super isEqual:obj];
}

- (NSString*)description {
  return [NSString
      stringWithFormat:@"<%@: %p; pattern = %@>", NSStringFromClass(self.class),
                       self,
                       base::SysUTF8ToNSString(url_pattern->GetAsString())];
}

- (id)copyWithZone:(NSZone*)zone {
  auto other = [[URLPatternIOS alloc] initWithValidSchemes:self.validSchemes];
  other.scheme = self.scheme;
  other.host = self.host;
  other.port = self.port;
  other.path = self.path;
  other.isMatchingSubdomains = self.isMatchingSubdomains;
  other.isMatchingAllURLs = self.isMatchingAllURLs;
  return other;
}

- (URLPatternIOSParseResult)parsePattern:(NSString*)pattern {
  return static_cast<URLPatternIOSParseResult>(
      url_pattern->Parse(base::SysNSStringToUTF8(pattern)));
}

- (URLPatternIOSScheme)validSchemes {
  return IOSSchemeForSchemeMask(url_pattern->valid_schemes());
}

- (void)setValidSchemes:(URLPatternIOSScheme)schemes {
  url_pattern->SetValidSchemes(SchemeMasksForIOSScheme(schemes));
}

- (NSString*)scheme {
  return base::SysUTF8ToNSString(url_pattern->scheme());
}

- (void)setScheme:(NSString*)scheme {
  url_pattern->SetScheme(base::SysNSStringToUTF8(scheme));
}

- (NSString*)host {
  return base::SysUTF8ToNSString(url_pattern->host());
}

- (void)setHost:(NSString*)host {
  url_pattern->SetHost(base::SysNSStringToUTF8(host));
}

- (NSString*)port {
  return base::SysUTF8ToNSString(url_pattern->port());
}

- (void)setPort:(NSString*)port {
  url_pattern->SetPort(base::SysNSStringToUTF8(port));
}

- (NSString*)path {
  return base::SysUTF8ToNSString(url_pattern->path());
}

- (void)setPath:(NSString*)path {
  url_pattern->SetPath(base::SysNSStringToUTF8(path));
}

- (bool)isMatchingSubdomains {
  return url_pattern->match_subdomains();
}

- (void)setIsMatchingSubdomains:(bool)matchingSubdomains {
  url_pattern->SetMatchSubdomains(matchingSubdomains);
}

- (bool)isMatchingAllURLs {
  return url_pattern->match_all_urls();
}

- (void)setIsMatchingAllURLs:(bool)matchingAllURLs {
  url_pattern->SetMatchAllURLs(matchingAllURLs);
}

- (bool)isValidScheme:(NSString*)scheme {
  return url_pattern->IsValidScheme(base::SysNSStringToUTF8(scheme));
}

- (bool)matchesURL:(NSURL*)url {
  return url_pattern->MatchesURL(net::GURLWithNSURL(url));
}

- (bool)matchesSecurityOrigin:(NSURL*)origin {
  return url_pattern->MatchesSecurityOrigin(net::GURLWithNSURL(origin));
}

- (bool)matchesScheme:(NSString*)scheme {
  return url_pattern->MatchesScheme(base::SysNSStringToUTF8(scheme));
}

- (bool)matchesHost:(NSString*)host {
  return url_pattern->MatchesHost(base::SysNSStringToUTF8(host));
}

- (bool)matchesPath:(NSString*)path {
  return url_pattern->MatchesPath(base::SysNSStringToUTF8(path));
}

- (bool)matchesSingleOrigin {
  return url_pattern->MatchesSingleOrigin();
}

- (bool)containsOtherURLPattern:(URLPatternIOS*)pattern {
  if (!pattern) {
    return false;
  }
  return url_pattern->Contains(*pattern->url_pattern);
}

- (bool)overlapsWithOtherURLPattern:(URLPatternIOS*)pattern {
  if (!pattern) {
    return false;
  }
  return url_pattern->OverlapsWith(*pattern->url_pattern);
}

@end
