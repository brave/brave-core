/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_INFO_H_

#include <string>

namespace brave_ads {

struct VerifiableConversionInfo final {
  [[nodiscard]] bool IsValid() const;

  std::string id;
  std::string advertiser_public_key_base64;
};

bool operator==(const VerifiableConversionInfo&,
                const VerifiableConversionInfo&);
bool operator!=(const VerifiableConversionInfo&,
                const VerifiableConversionInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_VERIFIABLE_CONVERSION_INFO_H_
