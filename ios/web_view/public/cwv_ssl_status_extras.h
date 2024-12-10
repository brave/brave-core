// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_SSL_STATUS_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_SSL_STATUS_EXTRAS_H_

#import <Foundation/Foundation.h>

#import "cwv_ssl_status.h"

NS_ASSUME_NONNULL_BEGIN

/// Describes an error that happened while showing a page over SSL.
OBJC_EXPORT
@interface CWVSSLErrorInformation : NSObject

/// A description of the error.
@property(readonly) NSString* details;

/// A short message describing the error (1 line).
@property(readonly) NSString* shortDescription;

@end

OBJC_EXPORT
@interface CWVSSLStatus (Extras)

/// Whether or not `certStatus` is an error
///
/// It is possible to have `securityStyle` be `authenticationBroken` and
/// non-error `certStatus` for WKWebView because `securityStyle` and
/// `certStatus` are calculated using different API, which may lead to different
/// cert verification results.  Check this before using error information from
/// `certStatusErrorsForURL`
@property(readonly) BOOL isCertStatusError;

/// A list of error details for a given URL using this SSL certificate
- (NSArray<CWVSSLErrorInformation*>*)certStatusErrorsForURL:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_SSL_STATUS_EXTRAS_H_
