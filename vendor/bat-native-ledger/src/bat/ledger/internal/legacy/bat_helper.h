/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_HELPER_H_
#define BRAVELEDGER_BAT_HELPER_H_

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"

namespace braveledger_bat_helper {

bool getJSONValue(const std::string& fieldName,
                  const std::string& json,
                  std::string* value);

bool getJSONTwitchProperties(
    const std::string& json,
    std::vector<std::map<std::string, std::string>>* parts);

std::string getBase64(const std::vector<uint8_t>& in);

// Sign using ed25519 algorithm
std::string sign(
    const std::vector<std::string>& keys,
    const std::vector<std::string>& values,
    const std::string& key_id,
    const std::vector<uint8_t>& secretKey);

bool HasSameDomainAndPath(
    const std::string& url,
    const std::string& to_match,
    const std::string& path);

}  // namespace braveledger_bat_helper

#endif  // BRAVELEDGER_BAT_HELPER_H_
