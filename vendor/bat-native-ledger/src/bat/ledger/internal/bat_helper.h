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

#include "bat/ledger/internal/properties/wallet_info_properties.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"

namespace braveledger_bat_helper {
bool isProbiValid(const std::string& number);

using SaveVisitSignature = void(const std::string&, uint64_t);
using SaveVisitCallback = std::function<SaveVisitSignature>;

bool getJSONValue(const std::string& fieldName,
                  const std::string& json,
                  std::string* value);

bool getJSONList(const std::string& fieldName,
                 const std::string& json,
                 std::vector<std::string>* value);

bool getJSONWalletInfo(const std::string& json,
                       ledger::WalletInfoProperties* walletInfo,
                       double* fee_amount);

bool getJSONRates(const std::string& json,
                  std::map<std::string, double>* rates);

bool getJSONTwitchProperties(
    const std::string& json,
    std::vector<std::map<std::string, std::string>>* parts);

bool getJSONBatchSurveyors(const std::string& json,
                           std::vector<std::string>* surveyors);

bool getJSONRecoverWallet(const std::string& json,
                          double* balance);

bool getJSONResponse(const std::string& json,
                     unsigned int* statusCode,
                     std::string* error);

bool getJSONAddresses(const std::string& json,
                      std::map<std::string, std::string>* addresses);

bool getJSONMessage(const std::string& json,
                     std::string* message);

std::vector<uint8_t> generateSeed();

std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed);

bool getPublicKeyFromSeed(const std::vector<uint8_t>& seed,
                          std::vector<uint8_t>* publicKey,
                          std::vector<uint8_t>* secretKey);

std::string uint8ToHex(const std::vector<uint8_t>& in);

std::string stringify(std::string* keys,
                      std::string* values,
                      const unsigned int size);

std::vector<uint8_t> getSHA256(const std::string& in);

std::string getBase64(const std::vector<uint8_t>& in);

bool getFromBase64(const std::string& in, std::vector<uint8_t>* out);

// Sign using ed25519 algorithm
std::string sign(
    const std::vector<std::string>& keys,
    const std::vector<std::string>& values,
    const std::string& key_id,
    const std::vector<uint8_t>& secretKey);

uint8_t niceware_mnemonic_to_bytes(
    const std::string& w,
    std::vector<uint8_t>* bytes_out,
    size_t* written,
    std::vector<std::string> wordDictionary);

bool HasSameDomainAndPath(
    const std::string& url,
    const std::string& to_match,
    const std::string& path);

}  // namespace braveledger_bat_helper

#endif  // BRAVELEDGER_BAT_HELPER_H_
