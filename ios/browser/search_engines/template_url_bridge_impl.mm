// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/search_engines/template_url_bridge_impl.h"

#include "base/strings/sys_string_conversions.h"
#include "components/search_engines/template_url.h"
#include "net/base/apple/url_conversions.h"

@implementation TemplateURLBridge

- (instancetype)initWithTemplateURL:(const TemplateURL*)templateURL {
  if ((self = [super init])) {
    _syncGUID = base::SysUTF8ToNSString(templateURL->sync_guid());
    _shortName = base::SysUTF16ToNSString(templateURL->short_name());
    _keyword = base::SysUTF16ToNSString(templateURL->keyword());
    _url = base::SysUTF8ToNSString(templateURL->url());
    if (!templateURL->suggestions_url().empty()) {
      _suggestionsURL = base::SysUTF8ToNSString(templateURL->suggestions_url());
    }
    _faviconURL = net::NSURLWithGURL(templateURL->favicon_url());
    _prepopulated = templateURL->prepopulate_id() > 0;
  }
  return self;
}

@end
