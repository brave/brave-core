// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/url_sanitizer/ios/browser/url_sanitizer_service_bridge_impl.h"

#include "brave/components/url_sanitizer/core/browser/url_sanitizer_service.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation URLSanitizerServiceBridgeImpl

- (instancetype)initWithURLSanitizerService:
    (raw_ptr<brave::URLSanitizerService>)urlSanitizerService {
  if ((self = [super init])) {
    _urlSanitizerService = urlSanitizerService;
  }
  return self;
}

- (nullable NSURL*)sanitizeURL:(NSURL*)url {
  return net::NSURLWithGURL(
      self.urlSanitizerService->SanitizeURL(net::GURLWithNSURL(url)));
}

@end
