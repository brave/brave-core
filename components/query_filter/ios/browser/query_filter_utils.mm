// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/ios/browser/query_filter_utils.h"

#include <optional>

#include "base/strings/sys_string_conversions.h"
#include "brave/components/query_filter/core/browser/utils.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation QueryFilterUtils

+ (nullable NSURL*)applyQueryFilter:(NSURL*)url {
  std::optional<GURL> filteredURL =
      query_filter::ApplyQueryFilter(net::GURLWithNSURL(url));
  if (!filteredURL.has_value()) {
    return nil;
  }
  return net::NSURLWithGURL(filteredURL.value());
}

+ (nullable NSURL*)applyQueryStringFilter:(NSURL*)url
                             initiatorURL:(nullable NSURL*)initiatorURL
                        redirectSourceURL:(nullable NSURL*)redirectSourceURL
                            requestMethod:(NSString*)requestMethod
                       isInternalRedirect:(BOOL)isInternalRedirect {
  std::optional<GURL> filteredURL = query_filter::MaybeApplyQueryStringFilter(
      net::GURLWithNSURL(initiatorURL), net::GURLWithNSURL(redirectSourceURL),
      net::GURLWithNSURL(url), base::SysNSStringToUTF8(requestMethod),
      isInternalRedirect);

  if (!filteredURL.has_value()) {
    return nil;
  }
  return net::NSURLWithGURL(filteredURL.value());
}

@end
