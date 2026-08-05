/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESEND_VERIFICATION_EMAIL_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESEND_VERIFICATION_EMAIL_H_

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/flows/flow_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the verification-email resend of the `mojom::Authentication` surface.
// Unlike most flow helpers, this one is held by `StateBase` itself, since
// `ResendVerificationEmail()` is valid in both the logged-out and logged-in
// states: `StateBase` holds a `ResendVerificationEmail` member and forwards
// the single mojom `ResendVerificationEmail` method to `operator()()` here,
// which maps onto one backend endpoint:
//
//   operator()() -> /v2/verify/resend
//
// Requests are sent through the inherited `FlowBase` helpers, so their
// lifetime is tied to this flow (see `FlowBase::SendStateOwnedRequest`).
class ResendVerificationEmail : public FlowBase {
 public:
  ResendVerificationEmail(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor);

  ResendVerificationEmail(const ResendVerificationEmail&) = delete;
  ResendVerificationEmail& operator=(const ResendVerificationEmail&) = delete;

  ~ResendVerificationEmail();

  void operator()(
      mojom::VerificationIntentPtr intent,
      mojom::Authentication::ResendVerificationEmailCallback callback);

 private:
  void OnResponse(
      mojom::Authentication::ResendVerificationEmailCallback callback,
      endpoints::VerifyResend::Response response);

  base::WeakPtrFactory<ResendVerificationEmail> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESEND_VERIFICATION_EMAIL_H_
