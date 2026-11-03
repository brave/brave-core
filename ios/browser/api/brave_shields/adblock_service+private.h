/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_PRIVATE_H_

#include "brave/ios/browser/api/brave_shields/adblock_service.h"

class ProfileIOS;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

@interface AdblockService (Private)
- (instancetype)initWithComponentUpdater:
    (component_updater::ComponentUpdateService*)componentUpdaterService;
- (void)registerDefaultShieldsComponent;
/// Connects to `profile`'s shared AdBlockService when
/// `kBraveIOSSharedAdBlockService` is enabled.
- (void)attachToProfile:(ProfileIOS*)profile;
/// Must be called before the attached profile is destroyed.
- (void)detachFromProfile;
@end

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_PRIVATE_H_
