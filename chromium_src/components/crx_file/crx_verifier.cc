/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/crx_file/crx_verifier.h"

#include <utility>
#include <vector>

namespace {

// TODO(atuchin): replace this to the real key hash.
constexpr uint8_t kBravePublisherKeyHash[] = {
    0xd5, 0x7d, 0xbb, 0xe7, 0xc5, 0x93, 0x8a, 0x4c, 0x9c, 0x7a, 0x88,
    0xf0, 0x43, 0x4,  0x53, 0xf0, 0x7c, 0x32, 0x18, 0xf6, 0xc9, 0x74,
    0x82, 0xa5, 0x95, 0xa5, 0xa9, 0xac, 0x8c, 0xcf, 0x90, 0x14};

std::vector<uint8_t>& GetBravePublisherKey() {
  static std::vector<uint8_t> brave_publisher_key(
      std::begin(kBravePublisherKeyHash), std::end(kBravePublisherKeyHash));
  return brave_publisher_key;
}

// Used in the patch in crx_verifier.cc.
bool IsBravePublisher(const std::vector<uint8_t>& key_hash) {
  return GetBravePublisherKey() == key_hash;
}

}  // namespace

namespace crx_file {

void SetBravePublisherKeyForTesting(const std::vector<uint8_t>& test_key) {
  GetBravePublisherKey() = test_key;
}

}  // namespace crx_file

#include "src/components/crx_file/crx_verifier.cc"
