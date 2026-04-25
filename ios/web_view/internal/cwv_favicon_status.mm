// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_favicon_status.h"

#include "ios/web/public/favicon/favicon_status.h"
#include "net/base/apple/url_conversions.h"
#include "ui/gfx/image/image.h"

@interface CWVFaviconStatus ()
@property(nonatomic, copy, nullable) NSURL* url;
@property(nonatomic, copy, nullable) UIImage* image;
@end

@implementation CWVFaviconStatus

- (instancetype)initWithFaviconStatus:(web::FaviconStatus)status {
  if ((self = [super init])) {
    self.url = net::NSURLWithGURL(status.url);
    self.image = status.image.ToUIImage();
  }
  return self;
}

@end
