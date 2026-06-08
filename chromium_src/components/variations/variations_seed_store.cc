/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/variations/variations_seed_store.h"

#include <ranges>

#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/no_destructor.h"
#include "crypto/sign.h"

namespace variations {

// A non-anonymous class to friend with base::CurrentTestVendor.
class PublicKeyWrapper {
 public:
  static const crypto::keypair::PublicKey& GetPublicKey(
      const crypto::keypair::PublicKey& public_key);
};

}  // namespace variations

#define Verify(signature_algorithm, public_key, seed_bytes, signature)    \
  Verify(signature_algorithm, PublicKeyWrapper::GetPublicKey(public_key), \
         seed_bytes, signature)

#include <components/variations/variations_seed_store.cc>

#undef Verify

namespace variations {

// static
const crypto::keypair::PublicKey& PublicKeyWrapper::GetPublicKey(
    const crypto::keypair::PublicKey& public_key) {
  // Only kPublicKey should be passed here. This is a sanity check.
  DCHECK(std::ranges::equal(public_key.ToSubjectPublicKeyInfo(), kPublicKey));

  // If we are in a Chromium test, return the original public key to let those
  // tests check everything they need.
  if (base::CurrentTestVendor::Get() == base::TestVendor::kChromium) {
    return public_key;
  }

  static constexpr uint8_t kBravePublicKeyBytes[] = {
      0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
      0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
      0x42, 0x00, 0x04, 0xbb, 0x6e, 0xed, 0x61, 0xf1, 0xfb, 0xf5, 0x4c, 0xfe,
      0xda, 0x7b, 0xad, 0xb6, 0x18, 0x27, 0x42, 0xa2, 0xbd, 0x14, 0x95, 0xb5,
      0x11, 0x2d, 0xf4, 0xc4, 0x89, 0x63, 0x2f, 0x26, 0xa2, 0x18, 0xa1, 0x36,
      0xe5, 0x6f, 0x38, 0x45, 0x5d, 0x40, 0x9a, 0x2a, 0x07, 0xbd, 0xcc, 0x35,
      0x33, 0xa5, 0x51, 0xcf, 0x8d, 0xe8, 0xf7, 0x98, 0xa3, 0x69, 0xad, 0xe4,
      0x88, 0xf9, 0xa1, 0x60, 0xc2, 0x6f, 0x84,
  };

  static const base::NoDestructor<crypto::keypair::PublicKey> kBravePublicKey(
      [] {
        auto brave_public_key =
            crypto::keypair::PublicKey::FromSubjectPublicKeyInfo(
                kBravePublicKeyBytes);
        DCHECK(brave_public_key);
        return std::move(*brave_public_key);
      }());

  return *kBravePublicKey;
}

}  // namespace variations

#undef BRAVE_K_PUBLIC_KEY
