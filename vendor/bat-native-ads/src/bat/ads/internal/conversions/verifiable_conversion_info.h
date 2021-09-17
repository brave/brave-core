/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_INFO_H_

#include <string>

namespace ads {

struct VerifiableConversionInfo final {
  VerifiableConversionInfo();
  VerifiableConversionInfo(const VerifiableConversionInfo& info);
  ~VerifiableConversionInfo();

  bool operator==(const VerifiableConversionInfo& rhs) const;
  bool operator!=(const VerifiableConversionInfo& rhs) const;

  bool IsValid() const;

  std::string id;
  std::string public_key;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_INFO_H_
