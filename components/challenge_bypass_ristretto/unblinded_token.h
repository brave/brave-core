/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/value_or_result_box.h"
#include "brave/third_party/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class TokenPreimage;
class VerificationKey;

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) UnblindedToken {
  using CxxUnblindedTokenBox = rust::Box<cbr_cxx::UnblindedToken>;
  using CxxUnblindedTokenResultBox = rust::Box<cbr_cxx::UnblindedTokenResult>;
  using CxxUnblindedTokenValueOrResult =
      ValueOrResultBox<cbr_cxx::UnblindedToken,
                       CxxUnblindedTokenBox,
                       CxxUnblindedTokenResultBox>;
  using CxxUnblindedTokenRefData =
      base::RefCountedData<CxxUnblindedTokenValueOrResult>;

 public:
  explicit UnblindedToken(CxxUnblindedTokenBox);
  explicit UnblindedToken(CxxUnblindedTokenResultBox);
  UnblindedToken(const UnblindedToken&);
  UnblindedToken& operator=(const UnblindedToken&);
  UnblindedToken(UnblindedToken&&) noexcept;
  UnblindedToken& operator=(UnblindedToken&&) noexcept;
  ~UnblindedToken();

  const cbr_cxx::UnblindedToken& raw() const { return raw_->data.unwrap(); }

  VerificationKey DeriveVerificationKey() const;
  TokenPreimage Preimage() const;
  static base::expected<UnblindedToken, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const UnblindedToken& rhs) const;

 private:
  scoped_refptr<CxxUnblindedTokenRefData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_
