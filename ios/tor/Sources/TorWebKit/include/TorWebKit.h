// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN
/// Returns nil when the native timezone override is unavailable.
FOUNDATION_EXPORT WKProcessPool * _Nullable BraveTorUTCProcessPool(void);
/// Exposed for an independent non-UTC control in native WebKit tests.
FOUNDATION_EXPORT WKProcessPool * _Nullable BraveTorProcessPoolForTimeZone(NSString *timeZone);
/// Keeps universal links in WebKit rather than opening their associated apps.
FOUNDATION_EXPORT WKNavigationActionPolicy BraveTorInWebViewNavigationPolicy(void);
/// Reports whether AVFoundation supports WebKit's exclusive client loader.
FOUNDATION_EXPORT BOOL BraveTorSupportsExclusiveMediaLoading(void);
NS_ASSUME_NONNULL_END
