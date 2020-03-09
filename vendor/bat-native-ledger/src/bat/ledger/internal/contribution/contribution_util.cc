/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/contribution/contribution_util.h"

#include "wrapper.hpp"  // NOLINT

using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

namespace braveledger_contribution {

ledger::ReportType GetReportTypeFromRewardsType(
    const ledger::RewardsType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE): {
      return ledger::ReportType::AUTO_CONTRIBUTION;
    }
    case static_cast<int>(ledger::RewardsType::ONE_TIME_TIP): {
      return ledger::ReportType::TIP;
    }
    case static_cast<int>(ledger::RewardsType::RECURRING_TIP): {
      return ledger::ReportType::TIP_RECURRING;
    }
    default: {
      // missing conversion, returning dummy value.
      NOTREACHED();
      return ledger::ReportType::TIP;
    }
  }
}

bool GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& suggestion_encoded,
    base::Value* result) {
  if (token_value.empty() || public_key.empty() || suggestion_encoded.empty()) {
    return false;
  }

  UnblindedToken unblinded = UnblindedToken::decode_base64(token_value);
  VerificationKey verification_key = unblinded.derive_verification_key();
  VerificationSignature signature = verification_key.sign(suggestion_encoded);
  const std::string pre_image = unblinded.preimage().encode_base64();

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    return false;
  }

  result->SetStringKey("t", pre_image);
  result->SetStringKey("publicKey", public_key);
  result->SetStringKey("signature", signature.encode_base64());
  return true;
}

bool GenerateSuggestionMock(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& suggestion_encoded,
    base::Value* result) {
  result->SetStringKey("t", token_value);
  result->SetStringKey("publicKey", public_key);
  result->SetStringKey("signature", token_value);
  return true;
}

double GetTotalFromRecurringVerified(
    const ledger::PublisherInfoList& publisher_list) {
  double total_recurring_amount = 0.0;
  for (const auto& publisher : publisher_list) {
    if (publisher->id.empty()) {
      continue;
    }

    if (publisher->status == ledger::PublisherStatus::VERIFIED) {
      total_recurring_amount += publisher->weight;
    }
  }

  return total_recurring_amount;
}

}  // namespace braveledger_contribution
