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

OBJC_EXPORT
NS_SWIFT_NAME(URLFormatter)
@interface BraveURLFormatter : NSObject
- (instancetype)init NS_UNAVAILABLE;
+ (NSString*)formatURLOriginForSecurityDisplay:(NSString*)origin
                                 schemeDisplay:
                                     (BraveURLSchemeDisplay)schemeDisplay;
+ (NSString*)formatURLOriginForDisplayOmitSchemePathAndTrivialSubdomains:
    (NSString*)origin;

+ (NSString*)formatURL:(NSString*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_URL_FORMATTER_H_
