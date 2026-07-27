/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_summary.h"

#include <optional>
#include <string_view>

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"

namespace brave_vpn::v2 {

// static
std::optional<CredentialSummary> CredentialSummary::FromMessage(
    std::string_view message) {
  const std::string_view trimmed =
      base::TrimWhitespaceASCII(message, base::TrimPositions::TRIM_ALL);

  // Empty/whitespace-only body: no subscription on record.
  if (trimmed.empty()) {
    return CredentialSummary{};
  }

  std::optional<base::DictValue> records =
      base::JSONReader::ReadDict(trimmed, base::JSON_PARSE_RFC);
  if (!records) {
    // Non-empty body that does not parse as a JSON object: genuine failure.
    return std::nullopt;
  }

  // Parsed, but to an empty object: also "no subscription on record".
  if (records->empty()) {
    return CredentialSummary{};
  }

  CredentialSummary summary;
  summary.empty_ = false;
  summary.active_ =
      records->FindBool(kCredentialSummaryActiveKey).value_or(false);
  summary.remaining_credential_count_ =
      records->FindInt(kCredentialSummaryRemainingCredentialCountKey)
          .value_or(0);
  return summary;
}

}  // namespace brave_vpn::v2
