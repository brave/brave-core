/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/value_or_result_box.h"
#include "brave/third_party/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) TokenPreimage {
  using CxxTokenPreimageBox = rust::Box<cbr_cxx::TokenPreimage>;
  using CxxTokenPreimageResultBox = rust::Box<cbr_cxx::TokenPreimageResult>;
  using CxxTokenPreimageValueOrResult =
      ValueOrResultBox<cbr_cxx::TokenPreimage,
                       CxxTokenPreimageBox,
                       CxxTokenPreimageResultBox>;
  using CxxTokenPreimageRefData =
      base::RefCountedData<CxxTokenPreimageValueOrResult>;

 public:
  explicit TokenPreimage(CxxTokenPreimageBox raw);
  explicit TokenPreimage(CxxTokenPreimageResultBox raw);
  TokenPreimage(const TokenPreimage&);
  TokenPreimage& operator=(const TokenPreimage&);
  TokenPreimage(TokenPreimage&&) noexcept;
  TokenPreimage& operator=(TokenPreimage&&) noexcept;
  ~TokenPreimage();

  const cbr_cxx::TokenPreimage& raw() const { return raw_->data.unwrap(); }

  static base::expected<TokenPreimage, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const TokenPreimage& rhs) const;

 private:
  scoped_refptr<CxxTokenPreimageRefData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_
