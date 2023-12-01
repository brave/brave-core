/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_KEY_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_KEY_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/third_party/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class VerificationSignature;

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) VerificationKey {
  using CxxVerificationKeyBox = rust::Box<cbr_cxx::VerificationKey>;
  using CxxVerificationKeyRefData = base::RefCountedData<CxxVerificationKeyBox>;

 public:
  explicit VerificationKey(CxxVerificationKeyBox raw);
  VerificationKey(const VerificationKey&);
  VerificationKey& operator=(const VerificationKey&);
  VerificationKey(VerificationKey&&) noexcept;
  VerificationKey& operator=(VerificationKey&&) noexcept;
  ~VerificationKey();

  const cbr_cxx::VerificationKey& raw() const { return *raw_->data; }

  base::expected<VerificationSignature, std::string> Sign(
      const std::string& message);
  base::expected<bool, std::string> Verify(const VerificationSignature& sig,
                                           const std::string& message);

 private:
  scoped_refptr<CxxVerificationKeyRefData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_KEY_H_
