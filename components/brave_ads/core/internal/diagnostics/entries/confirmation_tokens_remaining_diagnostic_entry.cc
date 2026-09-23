/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/confirmation_tokens_remaining_diagnostic_entry.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Confirmation tokens remaining";
}  // namespace

DiagnosticEntryType ConfirmationTokensRemainingDiagnosticEntry::GetType()
    const {
  return DiagnosticEntryType::kConfirmationTokensRemaining;
}

std::string ConfirmationTokensRemainingDiagnosticEntry::GetName() const {
  return kName;
}

std::string ConfirmationTokensRemainingDiagnosticEntry::GetValue() const {
  return base::NumberToString(ConfirmationTokenCount());
}

}  // namespace brave_ads
