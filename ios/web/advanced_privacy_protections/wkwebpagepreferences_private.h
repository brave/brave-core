// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_ADVANCED_PRIVACY_PROTECTIONS_WKWEBPAGEPREFERENCES_PRIVATE_H_
#define BRAVE_IOS_WEB_ADVANCED_PRIVACY_PROTECTIONS_WKWEBPAGEPREFERENCES_PRIVATE_H_

#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface WKWebpagePreferences (PrivateExpose)

@property(nonatomic) BOOL _networkConnectionIntegrityEnabled;
- (void)_setNetworkConnectionIntegrityEnabled:(BOOL)enabled;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_ADVANCED_PRIVACY_PROTECTIONS_WKWEBPAGEPREFERENCES_PRIVATE_H_
