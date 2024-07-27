/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/app/brave_core_switches.h"

#include "base/base_switches.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "brave/components/p3a/switches.h"
#include "brave/components/variations/switches.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/sync/base/command_line_switches.h"
#include "components/variations/variations_switches.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const BraveCoreSwitchKey BraveCoreSwitchKeyComponentUpdater =
    base::SysUTF8ToNSString(switches::kComponentUpdater);
const BraveCoreSwitchKey BraveCoreSwitchKeyVModule =
    base::SysUTF8ToNSString(switches::kVModule);
const BraveCoreSwitchKey BraveCoreSwitchKeySyncURL =
    base::SysUTF8ToNSString(syncer::kSyncServiceURL);
const BraveCoreSwitchKey BraveCoreSwitchKeyVariationsPR =
    base::SysUTF8ToNSString(variations::switches::kVariationsPR);
const BraveCoreSwitchKey BraveCoreSwitchKeyVariationsURL =
    base::SysUTF8ToNSString(variations::switches::kVariationsServerURL);
// There is no exposed switch for rewards
const BraveCoreSwitchKey BraveCoreSwitchKeyRewardsFlags = @"rewards";
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AUploadIntervalSeconds =
    base::SysUTF8ToNSString(p3a::switches::kP3AUploadIntervalSeconds);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3ADoNotRandomizeUploadInterval =
    base::SysUTF8ToNSString(p3a::switches::kP3ADoNotRandomizeUploadInterval);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3ATypicalRotationIntervalSeconds =
    base::SysUTF8ToNSString(p3a::switches::kP3ATypicalRotationIntervalSeconds);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AExpressRotationIntervalSeconds =
    base::SysUTF8ToNSString(p3a::switches::kP3AExpressRotationIntervalSeconds);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3ASlowRotationIntervalSeconds =
    base::SysUTF8ToNSString(p3a::switches::kP3ASlowRotationIntervalSeconds);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AFakeTypicalStarEpoch =
    base::SysUTF8ToNSString(p3a::switches::kP3AFakeTypicalStarEpoch);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AFakeSlowStarEpoch =
    base::SysUTF8ToNSString(p3a::switches::kP3AFakeSlowStarEpoch);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AFakeExpressStarEpoch =
    base::SysUTF8ToNSString(p3a::switches::kP3AFakeExpressStarEpoch);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AJsonUploadServerURL =
    base::SysUTF8ToNSString(p3a::switches::kP3AJsonUploadUrl);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3ACreativeUploadServerURL =
    base::SysUTF8ToNSString(p3a::switches::kP3ACreativeUploadUrl);
const BraveCoreSwitchKey BraveCoreSwitchKeyP2AJsonUploadServerURL =
    base::SysUTF8ToNSString(p3a::switches::kP2AJsonUploadUrl);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AConstellationUploadServerHost =
    base::SysUTF8ToNSString(p3a::switches::kP3AConstellationUploadHost);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3ADisableStarAttestation =
    base::SysUTF8ToNSString(p3a::switches::kP3ADisableStarAttestation);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AStarRandomnessHost =
    base::SysUTF8ToNSString(p3a::switches::kP3AStarRandomnessHost);
const BraveCoreSwitchKey BraveCoreSwitchKeyP3AIgnoreServerErrors =
    base::SysUTF8ToNSString(p3a::switches::kP3AIgnoreServerErrors);
const BraveCoreSwitchKey BraveCoreSwitchKeyUseDevGoUpdater =
    base::SysUTF8ToNSString(brave_component_updater::kUseGoUpdateDev);
const BraveCoreSwitchKey BraveCoreSwitchKeyServicesEnvironment =
    @"brave-services-env";

@implementation BraveCoreSwitch
- (instancetype)initWithKey:(BraveCoreSwitchKey)key {
  return [self initWithKey:key value:nil];
}
- (instancetype)initWithKey:(BraveCoreSwitchKey)key
                      value:(nullable NSString*)value {
  if ((self = [super init])) {
    _key = [key copy];
    _value = [value copy];
  }
  return self;
}
@end
