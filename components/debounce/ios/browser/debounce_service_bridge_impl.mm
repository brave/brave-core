// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/ios/browser/debounce_service_bridge_impl.h"

#include "brave/components/debounce/core/browser/debounce_service.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation DebounceServiceBridgeImpl

- (instancetype)initWithDebounceService:
    (raw_ptr<debounce::DebounceService>)debounceService {
  if ((self = [super init])) {
    _debounceService = debounceService;
  }
  return self;
}

- (BOOL)isEnabled {
  return self.debounceService->IsEnabled();
}

- (void)setEnabled:(BOOL)enabled {
  self.debounceService->SetIsEnabled(enabled);
}

- (nullable NSURL*)debounceURL:(NSURL*)url {
  GURL finalURL;

  if (!self.debounceService->Debounce(net::GURLWithNSURL(url), &finalURL)) {
    return nil;
  }

  return net::NSURLWithGURL(finalURL);
}

@end
