/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_ANDROID_ATTEST_ANDROID_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_ANDROID_ATTEST_ANDROID_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

// Issues the Android Rewards attestation / captcha-solve requests against the
// Brave grant endpoint via APIRequestHelper.
//
// The flow is split into two public steps because obtaining the Play Integrity
// token (between them) requires a Java-only Google Play API:
//
//   StartAttestation: POST /v1/attestations/android      -> "uniqueValue" (201)
//   (Java fetches the Play Integrity token for the nonce)
//   AttestPaymentId:  PUT  /v1/attestations/android/{id} -> 200, then chains to
//                     POST /v3/captcha/solution/{id}/{c}  -> 200
//
// The PUT -> POST transition is entirely server-side, so it is chained inside
// AttestPaymentId rather than bounced back through JNI.

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_adaptive_captcha {

class AttestAndroid {
 public:
  // Returns the server-issued nonce, or the empty string on failure.
  using OnStartAttestation =
      base::OnceCallback<void(const std::string& unique_value)>;
  using OnAttestResult = base::OnceCallback<void(bool success)>;

  explicit AttestAndroid(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AttestAndroid();

  AttestAndroid(const AttestAndroid&) = delete;
  AttestAndroid& operator=(const AttestAndroid&) = delete;

  void StartAttestation(const std::string& payment_id,
                        OnStartAttestation callback);

  // Submits the Play Integrity token, then on success solves the captcha.
  // |callback| reports the final result.
  void AttestPaymentId(const std::string& payment_id,
                       const std::string& captcha_id,
                       const std::string& integrity_token,
                       const std::string& unique_value,
                       const std::string& package_name,
                       OnAttestResult callback);

 private:
  void OnStartAttestationResponse(
      OnStartAttestation callback,
      api_request_helper::APIRequestResult api_request_result);

  void OnAttestPaymentIdResponse(
      const std::string& payment_id,
      const std::string& captcha_id,
      OnAttestResult callback,
      api_request_helper::APIRequestResult api_request_result);

  void SolveCaptcha(const std::string& payment_id,
                    const std::string& captcha_id,
                    OnAttestResult callback);

  void OnSolveCaptchaResponse(
      OnAttestResult callback,
      api_request_helper::APIRequestResult api_request_result);

  api_request_helper::APIRequestHelper api_request_helper_;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_ANDROID_ATTEST_ANDROID_H_
