/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/value_or_result_box.h"
#include "brave/third_party/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class BlindedToken;
class PublicKey;
class SignedToken;
class TokenPreimage;
class UnblindedToken;

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) SigningKey {
  using CxxSigningKeyBox = rust::Box<cbr_cxx::SigningKey>;
  using CxxSigningKeyResultBox = rust::Box<cbr_cxx::SigningKeyResult>;
  using CxxSigningKeyValueOrResult = ValueOrResultBox<cbr_cxx::SigningKey,
                                                      CxxSigningKeyBox,
                                                      CxxSigningKeyResultBox>;
  using CxxSigningKeyRefData = base::RefCountedData<CxxSigningKeyValueOrResult>;

 public:
  explicit SigningKey(CxxSigningKeyBox raw);
  explicit SigningKey(CxxSigningKeyResultBox raw);
  SigningKey(const SigningKey&);
  SigningKey& operator=(const SigningKey&);
  SigningKey(SigningKey&&) noexcept;
  SigningKey& operator=(SigningKey&&) noexcept;
  ~SigningKey();

  const cbr_cxx::SigningKey& raw() const { return raw_->data.unwrap(); }

  static SigningKey Random();
  base::expected<SignedToken, std::string> Sign(
      const BlindedToken& blinded_token) const;
  UnblindedToken RederiveUnblindedToken(const TokenPreimage& t);
  PublicKey GetPublicKey();
  static base::expected<SigningKey, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const SigningKey& rhs) const;

 private:
  scoped_refptr<CxxSigningKeyRefData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_
