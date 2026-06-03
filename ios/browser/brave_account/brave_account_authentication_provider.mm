/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_authentication_provider.h"

#include <utility>

#include "base/apple/foundation_util.h"
#include "base/check_deref.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.objc+private.h"
#include "brave/ios/browser/api/profile/profile_bridge.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

@implementation BraveAccountAuthenticationProvider

+ (id<BraveAccountAuthentication>)authenticationForProfile:
    (id<ProfileBridge>)profileBridge {
  ProfileBridgeImpl* holder =
      base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge);
  auto* brave_account_service =
      brave_account::BraveAccountServiceFactoryIOS::GetFor(holder.profile);
  mojo::PendingRemote<brave_account::mojom::Authentication> pending_remote;
  CHECK_DEREF(brave_account_service)
      .BindInterface(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveAccountAuthenticationMojoImpl alloc]
      initWithAuthentication:std::move(pending_remote)];
}

@end
