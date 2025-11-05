/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_authentication_bridge_impl.h"

#include "base/apple/foundation_util.h"
#include "base/check_deref.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "brave/ios/browser/api/profile/profile_bridge.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "mojo/public/cpp/bindings/remote.h"

NSString* const BraveAccountAuthenticationTokenPref =
    base::SysUTF8ToNSString(brave_account::prefs::kAuthenticationToken);
NSString* const BraveAccountVerificationTokenPref =
    base::SysUTF8ToNSString(brave_account::prefs::kVerificationToken);

@implementation BraveAccountAuthenticationBridgeImpl {
  mojo::Remote<brave_account::mojom::Authentication> _authentication;
}

- (instancetype)initWithProfile:(id<ProfileBridge>)profileBridge {
  if ((self = [super init])) {
    ProfileBridgeImpl* holder =
        base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge);
    auto* brave_account_service =
        brave_account::BraveAccountServiceFactoryIOS::GetFor(holder.profile);
    CHECK_DEREF(brave_account_service)
        .BindInterface(_authentication.BindNewPipeAndPassReceiver());
  }
  return self;
}

- (void)resendConfirmationEmail {
  _authentication->ResendConfirmationEmail();
}

- (void)cancelRegistration {
  _authentication->CancelRegistration();
}

- (void)logOut {
  _authentication->LogOut();
}

@end
