/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/value_or_result_box.h"
#include "brave/third_party/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) BlindedToken {
  using CxxBlindedTokenBox = rust::Box<cbr_cxx::BlindedToken>;
  using CxxBlindedTokenResultBox = rust::Box<cbr_cxx::BlindedTokenResult>;
  using CxxBlindedTokenValueOrResult =
      ValueOrResultBox<cbr_cxx::BlindedToken,
                       CxxBlindedTokenBox,
                       CxxBlindedTokenResultBox>;
  using CxxBlindedTokenData =
      base::RefCountedData<CxxBlindedTokenValueOrResult>;

 public:
  explicit BlindedToken(CxxBlindedTokenBox raw);
  explicit BlindedToken(CxxBlindedTokenResultBox raw);
  BlindedToken(const BlindedToken& other);
  BlindedToken& operator=(const BlindedToken& other);
  BlindedToken(BlindedToken&& other) noexcept;
  BlindedToken& operator=(BlindedToken&& other) noexcept;
  ~BlindedToken();

  const cbr_cxx::BlindedToken& raw() const { return raw_->data.unwrap(); }

  static base::expected<BlindedToken, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const BlindedToken& rhs) const;

 private:
  scoped_refptr<CxxBlindedTokenData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_
