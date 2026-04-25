// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_APP_BRAVE_PROFILE_CONTROLLER_PRIVATE_H_
#define BRAVE_IOS_APP_BRAVE_PROFILE_CONTROLLER_PRIVATE_H_

#include "base/memory/raw_ptr.h"
#include "brave/ios/app/brave_profile_controller.h"

class ScopedProfileKeepAliveIOS;

@interface BraveProfileController (Private)
- (instancetype)initWithProfileKeepAlive:
    (ScopedProfileKeepAliveIOS)profileKeepAlive;
@end

#endif  // BRAVE_IOS_APP_BRAVE_PROFILE_CONTROLLER_PRIVATE_H_
