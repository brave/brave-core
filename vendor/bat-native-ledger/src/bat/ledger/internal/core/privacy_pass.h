/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_PRIVACY_PASS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_PRIVACY_PASS_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// challenge_bypass_ristretto_ffi
#include "wrapper.hpp"

namespace ledger {

class PrivacyPass : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "privacy-pass";

  struct BlindedTokenBatch {
    BlindedTokenBatch();
    ~BlindedTokenBatch();

    BlindedTokenBatch(const BlindedTokenBatch& other);
    BlindedTokenBatch& operator=(const BlindedTokenBatch& other);

    std::vector<std::string> tokens;
    std::vector<std::string> blinded_tokens;
  };

  BlindedTokenBatch CreateBlindedTokens(size_t count);

  absl::optional<std::vector<std::string>> UnblindTokens(
      const std::vector<std::string>& tokens,
      const std::vector<std::string>& blinded_tokens,
      const std::vector<std::string>& signed_tokens,
      const std::string& batch_proof,
      const std::string& public_key);

  struct SignResult {
    std::string preimage;
    std::string signature;
  };

  absl::optional<SignResult> SignMessage(const std::string& unblinded_token,
                                         const std::string& message);

 private:
  bool ErrorOccurred();
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_PRIVACY_PASS_H_
