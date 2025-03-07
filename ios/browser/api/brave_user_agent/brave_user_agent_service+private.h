// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_USER_AGENT_BRAVE_USER_AGENT_SERVICE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_USER_AGENT_BRAVE_USER_AGENT_SERVICE_PRIVATE_H_

#include "brave/components/brave_user_agent/browser/brave_user_agent_service.h"
#include "brave/ios/browser/api/brave_user_agent/brave_user_agent_service.h"

@interface BraveUserAgentService (Private)
- (instancetype)initWithBraveUserAgentService:
    (brave_user_agent::BraveUserAgentService*)braveUserAgentService;
@end

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_USER_AGENT_BRAVE_USER_AGENT_SERVICE_PRIVATE_H_
