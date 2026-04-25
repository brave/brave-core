// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "brave/ios/browser/api/brave_user_agent/brave_user_agent_exceptions_ios+private.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "net/base/apple/url_conversions.h"
#include "net/base/features.h"
#include "url/gurl.h"
#include "url/origin.h"

@interface BraveUserAgentExceptionsIOS () {
  raw_ptr<brave_user_agent::BraveUserAgentExceptions>
      _braveUserAgentExceptions;  // NOT OWNED
}

@end

@implementation BraveUserAgentExceptionsIOS
- (instancetype)initWithBraveUserAgentExceptions:
    (brave_user_agent::BraveUserAgentExceptions*)braveUserAgentExceptions {
  if ((self = [super init])) {
    _braveUserAgentExceptions = braveUserAgentExceptions;
  }
  return self;
}

- (bool)canShowBrave:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  if (!base::FeatureList::IsEnabled(
          brave_user_agent::features::kUseBraveUserAgent) ||
      !gurl.is_valid()) {
    return true;
  }

  return _braveUserAgentExceptions->CanShowBrave(gurl);
}
@end
