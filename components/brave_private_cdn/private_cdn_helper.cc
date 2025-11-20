/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_private_cdn/private_cdn_helper.h"

#include <string_view>

#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "base/logging.h"

namespace brave::private_cdn {

bool RemovePadding(std::string_view* padded_string) {
  if (!padded_string) {
    return false;
  }
  LOG(INFO) << "###RemovePadding try: " << padded_string->size();

  if (padded_string->size() < sizeof(uint32_t)) {
    return false;  // Missing length field
  }

  // Read payload length from the header.
  uint32_t data_length =
      base::U32FromBigEndian(base::as_byte_span(*padded_string).first<4u>());

  // Remove length header.
  padded_string->remove_prefix(sizeof(uint32_t));
  if (padded_string->size() < data_length) {
    return false;  // Payload shorter than expected length
  }

  // Remove padding.
  padded_string->remove_suffix(padded_string->size() - data_length);
  LOG(INFO) << "###RemovePadding done: " << padded_string->size();
  return true;
}

}  // namespace brave::private_cdn
