/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P2A_BRAVE_P2A_PROTOCOLS_H_
#define BRAVE_COMPONENTS_P2A_BRAVE_P2A_PROTOCOLS_H_

#include <stdint.h>

namespace brave {

class DirectEncodingProtocol {
 public:
  DirectEncodingProtocol();
  ~DirectEncodingProtocol();

  static uint64_t Perturb(
      uint16_t bucket_count, uint64_t value);
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P2A_BRAVE_P2A_PROTOCOLS_H_
