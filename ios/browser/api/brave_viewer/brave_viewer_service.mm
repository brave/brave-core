// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/brave_viewer/brave_viewer_service+private.h"
#include <string>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_viewer/browser/core/brave_viewer_service.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

@interface BraveViewerService () {
  raw_ptr<brave_viewer::BraveViewerService> braveViewerService_;  // NOT OWNED
}

@end

@implementation BraveViewerService

- (instancetype)init {
  self = [super init];
  if (self) {
    braveViewerService_ = brave_viewer::BraveViewerService::GetInstance();
  }
  return self;
}

- (void)getTestScriptForURL:(NSURL*)url 
                   callback:(void (^)(NSString* _Nullable script)) callback {
  DCHECK(braveViewerService_);
  GURL gurl = net::GURLWithNSURL(url);
  GURL final_url;

  auto cb = base::BindOnce(^(const std::string test_script) {
    const auto script = base::SysUTF8ToNSString(test_script);
    callback(script);
  });

  braveViewerService_->GetTestScript(gurl, std::move(cb));
}

@end
