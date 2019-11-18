/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROMOTION_PROMOTION_UTIL_H_
#define BRAVELEDGER_PROMOTION_PROMOTION_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"
#include "base/values.h"

namespace braveledger_promotion {

std::string ParseOSToString(ledger::OperatingSystem os);

std::string ParseClientInfoToString(ledger::ClientInfoPtr info);

ledger::PromotionType ConvertStringToPromotionType(const std::string& type);

ledger::ReportType ConvertPromotionTypeToReportType(
    const ledger::PromotionType type);

bool ParseFetchResponse(
    const std::string& response,
    ledger::PromotionList* list);

std::string ParseClaimTokenResponse(
    const std::string& response);

void ParseSignedTokensResponse(
    const std::string& response,
    base::Value* result);

std::unique_ptr<base::ListValue> ParseStringToBaseList(
    const std::string& string_list);

bool UnBlindTokensMock(
    ledger::PromotionPtr promotion,
    std::vector<std::string>* unblinded_encoded_tokens);

bool VerifyPublicKey(const ledger::PromotionPtr promotion);

}  // namespace braveledger_promotion

#endif  // BRAVELEDGER_PROMOTION_PROMOTION_UTIL_H_
