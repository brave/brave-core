// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_IOS_BROWSER_QUERY_FILTER_UTILS_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_IOS_BROWSER_QUERY_FILTER_UTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// A protocol definining methods that may be used to strip query params from
/// URLs
@protocol QueryFilterUtilsProtocol

/// This function will return a new url stripping known tracking query params.
/// If nothing is to be stripped, a null value is returned.
+ (nullable NSURL*)applyQueryFilter:(NSURL*)url;

/// This function will return a new url stripping known tracking query params.
/// If nothing is to be stripped, a null value is returned.
///
/// `initiator_url` specifies the origin initiating the resource request.
/// If there were redirects, this is the url prior to any redirects.
/// `redirect_source_url` specifies the url that we are currently navigating
/// from, including any redirects that might have happened. `request_url`
/// specifies where we are navigating to. `request_method` indicates the HTTP
/// method of the request. `internal_redirect` indicates wether or not this is
/// an internal redirect or not. This function returns the url we should
/// redirect to or a `std::nullopt` value if nothing is changed.
+ (nullable NSURL*)applyQueryStringFilter:(NSURL*)url
                             initiatorURL:(nullable NSURL*)initiatorURL
                        redirectSourceURL:(nullable NSURL*)redirectSourceURL
                            requestMethod:(NSString*)requestMethod
                       isInternalRedirect:(BOOL)isInternalRedirect;
@end

OBJC_EXPORT
@interface QueryFilterUtils : NSObject <QueryFilterUtilsProtocol>
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_IOS_BROWSER_QUERY_FILTER_UTILS_H_
