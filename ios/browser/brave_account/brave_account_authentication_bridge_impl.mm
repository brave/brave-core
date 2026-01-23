/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_authentication_bridge_impl.h"

#include <utility>

#include "base/apple/foundation_util.h"
#include "base/check_deref.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "brave/ios/browser/api/profile/profile_bridge.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"
#include "components/grit/brave_components_strings.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"

NSString* const BraveAccountAuthenticationTokenPref = base::SysUTF8ToNSString(
    brave_account::prefs::kBraveAccountAuthenticationToken);
NSString* const BraveAccountEmailAddressPref =
    base::SysUTF8ToNSString(brave_account::prefs::kBraveAccountEmailAddress);
NSString* const BraveAccountVerificationTokenPref = base::SysUTF8ToNSString(
    brave_account::prefs::kBraveAccountVerificationToken);

namespace {

NSString* GetAlertTitle(
    const brave_account::mojom::ResendConfirmationEmailErrorPtr& error) {
  return l10n_util::GetNSString(
      !error ? IDS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS_TITLE
             : IDS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ERROR_TITLE);
}

NSString* GetAlertMessage(
    const brave_account::mojom::ResendConfirmationEmailErrorPtr& error) {
  static const auto kErrorStrings = base::MakeFixedFlatMap<
      brave_account::mojom::ResendConfirmationEmailErrorCode, int>({
      {brave_account::mojom::ResendConfirmationEmailErrorCode::
           kMaximumEmailSendAttemptsExceeded,
       IDS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED},
      {brave_account::mojom::ResendConfirmationEmailErrorCode::
           kEmailAlreadyVerified,
       IDS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED},
  });

  if (!error) {
    return l10n_util::GetNSString(
        IDS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS);
  }

  if (!error->netErrorOrHttpStatus) {
    // client-side error
    return l10n_util::GetNSStringF(
        IDS_BRAVE_ACCOUNT_CLIENT_ERROR,
        error->errorCode
            ? base::UTF8ToUTF16(absl::StrFormat(
                  " (%s=%d)", l10n_util::GetStringUTF8(IDS_BRAVE_ACCOUNT_ERROR),
                  static_cast<int>(*error->errorCode)))
            : u"");
  }

  // server-side error
  if (error->errorCode) {
    if (const auto* string_id =
            base::FindOrNull(kErrorStrings, *error->errorCode)) {
      return l10n_util::GetNSString(*string_id);
    }
  }

  return l10n_util::GetNSStringF(
      IDS_BRAVE_ACCOUNT_SERVER_ERROR,
      base::NumberToString16(*error->netErrorOrHttpStatus),
      error->errorCode
          ? base::UTF8ToUTF16(absl::StrFormat(
                ", %s=%d", l10n_util::GetStringUTF8(IDS_BRAVE_ACCOUNT_ERROR),
                static_cast<int>(*error->errorCode)))
          : u"");
}

}  // namespace

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

- (void)resendConfirmationEmail:(void (^)(NSString* alert_title,
                                          NSString* alert_message))callback {
  _authentication->ResendConfirmationEmail(base::BindOnce(
      ^(base::expected<brave_account::mojom::ResendConfirmationEmailResultPtr,
                       brave_account::mojom::ResendConfirmationEmailErrorPtr>
            result) {
        const auto error = std::move(result).error_or(nullptr);
        callback(GetAlertTitle(error), GetAlertMessage(error));
      }));
}

- (void)cancelRegistration {
  _authentication->CancelRegistration();
}

- (void)logOut {
  _authentication->LogOut();
}

@end
