// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/ios/browser/brave_origin/brave_origin_navigation_bridge.h"

class ProfileIOS;

namespace brave_origin {

// iOS delegate for BraveOriginService. Provides SKU service access and forwards
// other calls like OpenOriginSettings through BraveOriginNavigationBridge
class BraveOriginDelegateIOS : public BraveOriginService::Delegate {
 public:
  explicit BraveOriginDelegateIOS(ProfileIOS& profile);

  // BraveOriginService::Delegate:
  void OpenOriginSettings() override;
  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() override;

 private:
  const raw_ref<ProfileIOS> profile_;
};

}  // namespace brave_origin

NS_ASSUME_NONNULL_BEGIN

@interface BraveOriginNavigationBridge (Private)
+ (void)onOpenOriginSettings;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_BRIDGE_IMPL_H_
