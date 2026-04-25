// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_QUERY_FILTER_UTILS_H_
#define BRAVE_IOS_BROWSER_API_QUERY_FILTER_UTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NSURL (QueryFilterUtilities)

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
- (nullable NSURL*)
    brave_applyingQueryStringFilterWithInitiatorURL:
        (nullable NSURL*)initiatorURL
                                  redirectSourceURL:
                                      (nullable NSURL*)redirectSourceURL
                                      requestMethod:(NSString*)requestMethod
                                 isInternalRedirect:(BOOL)isInternalRedirect
    NS_SWIFT_NAME(applyingQueryFilter(initiatorURL:redirectSourceURL:requestMethod:isInternalRedirect:));  // NOLINT

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_QUERY_FILTER_UTILS_H_
