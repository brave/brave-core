// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/debounce/debounce_service+private.h"

#include "base/memory/raw_ptr.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

@interface DebounceService () {
  raw_ptr<debounce::DebounceService> debounceService_;  // NOT OWNED
}

@end

@implementation DebounceService

- (instancetype)initWithDebounceService:
    (debounce::DebounceService*)debounceService {
  if ((self = [super init])) {
    debounceService_ = debounceService;
  }

  return self;
}

- (bool)isEnabled {
  DCHECK(debounceService_);
  return debounceService_->IsEnabled();
}

- (void)setIsEnabled:(bool)isEnabled {
  debounceService_->SetIsEnabled(isEnabled);
}

- (nullable NSURL*)debounceURL:(NSURL*)url {
  DCHECK(debounceService_);
  GURL gurl = net::GURLWithNSURL(url);
  GURL final_url;

  if (debounceService_->Debounce(gurl, &final_url)) {
    return net::NSURLWithGURL(final_url);
  }

  return nullptr;
}

@end
