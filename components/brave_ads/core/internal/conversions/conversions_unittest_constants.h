/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_UNITTEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_UNITTEST_CONSTANTS_H_

namespace brave_ads {

constexpr char kConversionId[] = "smartbrownfoxes42";
constexpr char kInvalidConversionId[] = "smart brown foxes 16";
constexpr char kEmptyConversionId[] = "";

constexpr char kConversionAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
constexpr char kEmptyConversionAdvertiserPublicKey[] = "";

constexpr char kConversionAdvertiserSecretKey[] =
    "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";
constexpr char kInvalidConversionAdvertiserPublicKey[] = "INVALID";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_UNITTEST_CONSTANTS_H_
