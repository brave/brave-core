/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/crx_file/crx_verifier.h"

#include <utility>
#include <vector>

#include "base/containers/span.h"

namespace {

// The Brave publisher key that is accepted in addition to upstream's
// kPublisherKeyHash. This key may be used to verify updates of the browser
// itself. If you change this constant, then you will likely also need to change
// the associated file crx-private-key.der, which is not in Git.
constexpr uint8_t kBravePublisherKeyHash[] = {
    0x93, 0x74, 0xd6, 0x2a, 0x32, 0x76, 0x74, 0x74, 0xac, 0x99, 0xd9,
    0xc0, 0x55, 0xea, 0xf2, 0x6e, 0x10, 0x7,  0x45, 0x6,  0xb9, 0xd5,
    0x35, 0xc8, 0x35, 0x8,  0x28, 0x97, 0x5f, 0x7a, 0xc1, 0x97};

auto GetBravePublisherKeyHash() {
  static auto brave_publisher_key = std::to_array(kBravePublisherKeyHash);
  return base::span(brave_publisher_key);
}

// Used in the patch in crx_verifier.cc.
bool IsBravePublisher(base::span<const uint8_t> key_hash) {
  return GetBravePublisherKeyHash() == key_hash;
}

}  // namespace

namespace crx_file {

void SetBravePublisherKeyHashForTesting(base::span<const uint8_t> test_key) {
  GetBravePublisherKeyHash().copy_from(test_key);
}

}  // namespace crx_file

#include "src/components/crx_file/crx_verifier.cc"
