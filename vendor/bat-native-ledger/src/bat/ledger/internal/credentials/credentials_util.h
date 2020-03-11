/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_CREDENTIALS_UTIL_H_
#define BRAVELEDGER_CREDENTIALS_CREDENTIALS_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ledger/mojom_structs.h"

#include "wrapper.hpp"

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace braveledger_credentials {
  std::vector<Token> GenerateCreds(const int count);

  std::string GetCredsJSON(const std::vector<Token>& creds);

  std::vector<BlindedToken> GenerateBlindCreds(
      const std::vector<Token>& tokens);

  std::string GetBlindedCredsJSON(const std::vector<BlindedToken>& blinded);

  std::unique_ptr<base::ListValue> ParseStringToBaseList(
      const std::string& string_list);

  bool UnBlindCreds(
      const ledger::CredsBatch& creds,
      std::vector<std::string>* unblinded_encoded_creds,
      std::string* error);

  bool UnBlindCredsMock(
      const ledger::CredsBatch& creds,
      std::vector<std::string>* unblinded_encoded_creds);

}  // namespace braveledger_credentials

#endif  // BRAVELEDGER_CREDENTIALS_CREDENTIALS_UTIL_H_
