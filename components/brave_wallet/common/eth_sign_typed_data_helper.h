/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_SIGN_TYPED_DATA_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_SIGN_TYPED_DATA_HELPER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/gtest_prod_util.h"
#include "base/values.h"

namespace brave_wallet {

// Helper to prepare typed data message to sign following eip712
// https://eips.ethereum.org/EIPS/eip-712
class EthSignTypedDataHelper {
 public:
  enum class Version { kV3, kV4 };
  static std::unique_ptr<EthSignTypedDataHelper> Create(base::Value::Dict types,
                                                        Version version);

  ~EthSignTypedDataHelper();
  EthSignTypedDataHelper(const EthSignTypedDataHelper&) = delete;
  EthSignTypedDataHelper& operator=(const EthSignTypedDataHelper&) = delete;

  void SetTypes(base::Value::Dict types);
  void SetVersion(Version version);

  std::vector<uint8_t> GetTypeHash(const std::string& primary_type_name) const;
  std::optional<std::pair<std::vector<uint8_t>, base::Value::Dict>> HashStruct(
      const std::string& primary_type_name,
      const base::Value::Dict& data) const;
  std::optional<std::pair<std::vector<uint8_t>, base::Value::Dict>> EncodeData(
      const std::string& primary_type_name,
      const base::Value::Dict& data) const;
  static std::optional<std::vector<uint8_t>> GetTypedDataMessageToSign(
      base::span<const uint8_t> domain_hash,
      base::span<const uint8_t> primary_hash);
  std::optional<std::pair<std::vector<uint8_t>, base::Value::Dict>>
  GetTypedDataPrimaryHash(const std::string& primary_type_name,
                          const base::Value::Dict& message) const;
  std::optional<std::pair<std::vector<uint8_t>, base::Value::Dict>>
  GetTypedDataDomainHash(const base::Value::Dict& domain_separator) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(EthSignedTypedDataHelperUnitTest, EncodeTypes);
  FRIEND_TEST_ALL_PREFIXES(EthSignedTypedDataHelperUnitTest,
                           InvalidEncodeTypes);
  FRIEND_TEST_ALL_PREFIXES(EthSignedTypedDataHelperUnitTest, EncodeTypesArrays);
  FRIEND_TEST_ALL_PREFIXES(EthSignedTypedDataHelperUnitTest, EncodeField);

  explicit EthSignTypedDataHelper(base::Value::Dict types, Version version);

  void FindAllDependencyTypes(
      base::flat_map<std::string, base::Value>* known_types,
      const std::string& anchor_type_name) const;
  std::string EncodeType(const base::Value& type,
                         const std::string& type_name) const;
  std::string EncodeTypes(const std::string& primary_type_name) const;

  std::optional<std::vector<uint8_t>> EncodeField(
      const std::string& type,
      const base::Value& value) const;

  base::Value::Dict types_;
  Version version_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_SIGN_TYPED_DATA_HELPER_H_
