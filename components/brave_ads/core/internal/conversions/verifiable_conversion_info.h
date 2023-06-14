/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_INFO_H_

#include <string>

namespace brave_ads {

struct VerifiableConversionInfo final {
  bool operator==(const VerifiableConversionInfo&) const;
  bool operator!=(const VerifiableConversionInfo&) const;

  [[nodiscard]] bool IsValid() const;

  std::string id;
  std::string public_key;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_INFO_H_
