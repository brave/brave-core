// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"                  // NOLINT
#import "cwv_web_view_configuration.h"  // NOLINT

@class WKWebsiteDataStore;
@protocol ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@interface BraveWebViewConfiguration : CWVWebViewConfiguration

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

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_H_
