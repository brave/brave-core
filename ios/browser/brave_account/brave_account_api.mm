/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_api.h"

#include "base/check.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "mojo/public/cpp/bindings/remote.h"

NSString* const BraveAccountAuthenticationTokenPref =
    base::SysUTF8ToNSString(brave_account::prefs::kAuthenticationToken);
NSString* const BraveAccountVerificationTokenPref =
    base::SysUTF8ToNSString(brave_account::prefs::kVerificationToken);

@implementation BraveAccountAPI {
  mojo::Remote<brave_account::mojom::Authentication> _authentication;
}

- (instancetype)initWithProfile:(ProfileIOS*)profile {
  if ((self = [super init])) {
    auto* service =
        brave_account::BraveAccountServiceFactoryIOS::GetFor(profile);
    CHECK(service);
    service->BindInterface(_authentication.BindNewPipeAndPassReceiver());
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
