/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/crx_file/crx_verifier.h"

#include <array>
#include <utility>

#include "base/containers/span.h"

namespace {

// The Brave publisher key that is accepted in addition to upstream's
// kPublisherKeyHash. This key may be used to verify updates of the browser
// itself. If you change this constant, then you will likely also need to change
// the associated file crx-private-key.der, which is not in Git.
// Until May 2024, components were only signed with 0x93, 0x74, 0xd6... Since
// then, they are also signed with this new key. Now, the value here ensures
// that only binaries signed with the new key are accepted.
constexpr uint8_t kBravePublisherKeyHash[] = {
    0xb8, 0xb9, 0xd3, 0x85, 0xd5, 0x1d, 0x37, 0x9d, 0x92, 0x56, 0xa0,
    0xf0, 0xa7, 0xf5, 0x1b, 0xb0, 0x8e, 0x3e, 0xb5, 0x64, 0xab, 0x85,
    0xbd, 0x19, 0xd6, 0xff, 0x49, 0xa7, 0x35, 0x19, 0x84, 0xf7};

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
