/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PROCHLO_BRAVE_PROCHLO_CRYPTO_H_
#define BRAVE_COMPONENTS_BRAVE_PROCHLO_BRAVE_PROCHLO_CRYPTO_H_

#include <vector>

#include "base/macros.h"
#include "brave/components/brave_prochlo/prochlo_crypto.h"

namespace prochlo {

class BraveProchloCrypto : public Crypto {
 public:
  BraveProchloCrypto();

  // Load the public key for the Analyzer from bytes.
  bool load_analyzer_key_from_bytes(const std::vector<char>& bytes);

  // Load the public key for the Analyzer from bytes.
  bool load_shuffler_key_from_bytes(const std::vector<char>& bytes);

 private:
  // Load a public key returning the structure.
  EVP_PKEY* load_public_key_from_bytes(const std::vector<char>& bytes);

  DISALLOW_COPY_AND_ASSIGN(BraveProchloCrypto);
};

}  // namespace prochlo

#endif  // BRAVE_COMPONENTS_BRAVE_PROCHLO_BRAVE_PROCHLO_CRYPTO_H_
