/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_util.h"

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

namespace {

constexpr char kVerifiableConversion[] = "verifiable conversion";
constexpr char kDefaultConversion[] = "conversion";

}  // namespace

std::string ConversionTypeToString(const ConversionInfo& conversion) {
  return conversion.verifiable ? kVerifiableConversion : kDefaultConversion;
}

}  // namespace brave_ads
