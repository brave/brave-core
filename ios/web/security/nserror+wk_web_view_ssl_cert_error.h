// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_SECURITY_NSERROR_WK_WEB_VIEW_SSL_CERT_ERROR_H_
#define BRAVE_IOS_WEB_SECURITY_NSERROR_WK_WEB_VIEW_SSL_CERT_ERROR_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NSError (WKWebViewSSLCertError)
/// Returns YES if the receiver is a WKWebView SSL certificate error.
@property(nonatomic, readonly) BOOL isWKWebViewSSLCertError;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_SECURITY_NSERROR_WK_WEB_VIEW_SSL_CERT_ERROR_H_
