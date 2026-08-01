/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/resend_verification_email.h"

#include <utility>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/mojom/resend_verification_email.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "brave/components/brave_account/state_internal.h"
#include "net/http/http_status_code.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::VerifyResend;
using internal::MakeRequest;
using internal::MakeServerError;

ResendVerificationEmail::ResendVerificationEmail(StateBase& state)
    : state_(state) {}

ResendVerificationEmail::~ResendVerificationEmail() = default;

void ResendVerificationEmail::operator()(
    mojom::VerificationIntentPtr intent,
    mojom::Authentication::ResendVerificationEmailCallback callback) {
  auto verification_token =
      state_
          ->GetDecryptedVerificationToken<mojom::ResendVerificationEmailError>(
              std::move(intent));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request = MakeRequest<WithHeaders<VerifyResend::Request>>();
  SetBearerToken(request, *verification_token);
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.body.locale = "";
  request.timeout_duration = kVerifyResendTimeout;

  state_->SendStateOwnedRequest<VerifyResend>(
      std::move(request),
      base::BindOnce(&ResendVerificationEmail::OnResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ResendVerificationEmail::OnResponse(
    mojom::Authentication::ResendVerificationEmailCallback callback,
    VerifyResend::Response response) {
  if (response.status_code == net::HTTP_NO_CONTENT) {
    return std::move(callback).Run(mojom::ResendVerificationEmailResult::New());
  }

  if (!response.body || response.body->has_value()) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResendVerificationEmailError>(
            response.status_code.value_or(response.net_error),
            mojom::ResendVerificationEmailServerErrorCode::kInvalidResponse)));
  }

  std::move(callback).Run(
      base::unexpected(MakeServerError<mojom::ResendVerificationEmailError>(
          CHECK_DEREF(response.status_code),
          std::move(response.body->error()))));
}

}  // namespace brave_account
