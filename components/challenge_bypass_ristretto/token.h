/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_TOKEN_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_TOKEN_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/challenge_bypass_ristretto/value_or_result_box.h"
#include "brave/third_party/rust/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) Token {
  using CxxTokenBox = rust::Box<cbr_cxx::Token>;
  using CxxTokenResultBox = rust::Box<cbr_cxx::TokenResult>;
  using CxxTokenValueOrResult =
      ValueOrResultBox<cbr_cxx::Token, CxxTokenBox, CxxTokenResultBox>;
  using CxxTokenRefData = base::RefCountedData<CxxTokenValueOrResult>;

 public:
  explicit Token(CxxTokenBox raw);
  explicit Token(CxxTokenResultBox raw);
  Token(const Token&);
  Token& operator=(const Token&);
  Token(Token&&) noexcept;
  Token& operator=(Token&&) noexcept;
  ~Token();

  const cbr_cxx::Token& raw() const { return raw_->data.unwrap(); }

  static Token Random();
  BlindedToken Blind();
  static base::expected<Token, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const Token& rhs) const;

 private:
  scoped_refptr<CxxTokenRefData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_TOKEN_H_
