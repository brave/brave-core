/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_CONVERSION_USER_DATA_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_CONVERSION_USER_DATA_CONSTANTS_H_

namespace brave_ads {

inline constexpr char kConversionKey[] = "conversion";

inline constexpr char kConversionActionTypeKey[] = "action";

inline constexpr char kVerifiableConversionEnvelopeKey[] = "envelope";
inline constexpr char kVerifiableConversionEnvelopeAlgorithmKey[] = "alg";
inline constexpr char kVerifiableConversionEnvelopeCipherTextKey[] =
    "ciphertext";
inline constexpr char kVerifiableConversionEnvelopeEphemeralPublicKeyKey[] =
    "epk";
inline constexpr char kVerifiableConversionEnvelopeNonceKey[] = "nonce";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_CONVERSION_USER_DATA_CONSTANTS_H_
