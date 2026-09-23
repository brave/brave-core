// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"                  // NOLINT
#import "cwv_web_view_configuration.h"  // NOLINT

@class CWVPreferences;
@class WKWebsiteDataStore;
@protocol ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@interface BraveWebViewConfiguration : CWVWebViewConfiguration

/// The profile associated with this web view configuration
@property(readonly) id<ProfileBridge> profile;

/// Exposes the website data store associated with the WKWebViewConfiguration
///
/// Note: Usage of this should be limited to when the
/// `UseProfileWebViewConfiguration` feature flag is enabled as the data store
/// returned by this method won't be the one that is passed in when creating
/// the `BraveWebView`
@property(readonly) WKWebsiteDataStore* websiteDataStore;

/// Obtain a BraveWebViewConfiguration for a given profile
+ (BraveWebViewConfiguration*)configurationForProfile:
    (id<ProfileBridge>)profileBridge NS_SWIFT_NAME(init(profile:));

@property(nonatomic, readonly) CWVPreferences* preferences NS_UNAVAILABLE;

@end

CWV_EXPORT
@interface BraveWebViewConfiguration (SkusJS)
// Set a closure to be executed when a Brave Account page requests a SKUs
// credential summary for a given domain. `message` is the result from the skus
// service.
- (void)setSkusCredentialsFetchedCallback:(void (^)(NSString* domain,
                                                    NSString* message))callback;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_H_
