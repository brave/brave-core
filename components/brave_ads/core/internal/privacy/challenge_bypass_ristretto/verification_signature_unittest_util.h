/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_UNITTEST_UTIL_H_

namespace brave_ads::privacy::cbr {

class VerificationSignature;

VerificationSignature GetVerificationSignatureForTesting();
VerificationSignature GetInvalidVerificationSignatureForTesting();

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_UNITTEST_UTIL_H_
