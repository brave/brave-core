/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CANCEL_VERIFICATION_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CANCEL_VERIFICATION_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/flows/flow_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the verification cancellation of the `mojom::Authentication` surface.
// Like `ResendVerificationEmail`, this one is held by `StateBase` itself, since
// `CancelVerification()` is valid in both the logged-out and logged-in states:
// `StateBase` holds a `CancelVerification` member and forwards the single mojom
// `CancelVerification` method to `operator()()` here, which best-effort
// notifies one backend endpoint before dropping the verification slot:
//
//   operator()() -> /v2/verify/delete
//
// The request is sent through the inherited `FlowBase` helpers, so its
// lifetime is tied to this flow (see `FlowBase::SendUnownedRequest`).
class CancelVerification : public FlowBase {
 public:
  CancelVerification(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor);

  CancelVerification(const CancelVerification&) = delete;
  CancelVerification& operator=(const CancelVerification&) = delete;

  ~CancelVerification();

  void operator()(mojom::VerificationIntentPtr intent);
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CANCEL_VERIFICATION_H_
