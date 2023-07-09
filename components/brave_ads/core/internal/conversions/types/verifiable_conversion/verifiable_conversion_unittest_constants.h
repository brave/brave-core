/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_UNITTEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_UNITTEST_CONSTANTS_H_

namespace brave_ads {

constexpr char kVerifiableConversionId[] = "smartbrownfoxes42";
constexpr char kInvalidVerifiableConversionId[] = "smart brown foxes 16";
constexpr char kEmptyVerifiableConversionId[] = "";

constexpr char kVerifiableConversionAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
constexpr char kInvalidVerifiableConversionAdvertiserPublicKey[] = "INVALID";
constexpr char kEmptyVerifiableConversionAdvertiserPublicKey[] = "";
constexpr char kVerifiableConversionAdvertiserSecretKey[] =
    "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_UNITTEST_CONSTANTS_H_
