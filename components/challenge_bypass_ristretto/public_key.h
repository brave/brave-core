/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/value_or_result_box.h"
#include "brave/third_party/rust/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) PublicKey {
  using CxxPublicKeyBox = rust::Box<cbr_cxx::PublicKey>;
  using CxxPublicKeyResultBox = rust::Box<cbr_cxx::PublicKeyResult>;
  using CxxPublicKeyValueOrResult = ValueOrResultBox<cbr_cxx::PublicKey,
                                                     CxxPublicKeyBox,
                                                     CxxPublicKeyResultBox>;
  using CxxPublicKeyData = base::RefCountedData<CxxPublicKeyValueOrResult>;

 public:
  explicit PublicKey(CxxPublicKeyBox raw);
  explicit PublicKey(CxxPublicKeyResultBox raw);
  PublicKey(const PublicKey&);
  PublicKey& operator=(const PublicKey&);
  PublicKey(PublicKey&&) noexcept;
  PublicKey& operator=(PublicKey&&) noexcept;
  ~PublicKey();

  const cbr_cxx::PublicKey& raw() const { return raw_->data.unwrap(); }

  static base::expected<PublicKey, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const PublicKey& rhs) const;

 private:
  scoped_refptr<CxxPublicKeyData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
