/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_UNITTEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_UNITTEST_CONSTANTS_H_

namespace brave_ads {

inline constexpr char kVerifiableConversionId[] = "smartbrownfoxes42";
inline constexpr char kInvalidVerifiableConversionId[] = "smart brown foxes 16";
inline constexpr char kEmptyVerifiableConversionId[] = "";

inline constexpr char kVerifiableConversionAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
inline constexpr char kInvalidVerifiableConversionAdvertiserPublicKey[] =
    "INVALID";
inline constexpr char kEmptyVerifiableConversionAdvertiserPublicKey[] = "";
inline constexpr char kVerifiableConversionAdvertiserSecretKey[] =
    "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_UNITTEST_CONSTANTS_H_
