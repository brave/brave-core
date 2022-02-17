/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_SIGN_TYPED_DATA_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_SIGN_TYPED_DATA_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// Helper to prepare typed data message to sign following eip712
// https://eips.ethereum.org/EIPS/eip-712
class EthSignTypedDataHelper {
 public:
  enum class Version { kV3, kV4 };
  static std::unique_ptr<EthSignTypedDataHelper> Create(
      const base::Value& types,
      Version version);

  ~EthSignTypedDataHelper();
  EthSignTypedDataHelper(const EthSignTypedDataHelper&) = delete;
  EthSignTypedDataHelper& operator=(const EthSignTypedDataHelper&) = delete;

  bool SetTypes(const base::Value& types);
  void SetVersion(Version version);

  std::vector<uint8_t> GetTypeHash(const std::string primary_type_name) const;
  absl::optional<std::vector<uint8_t>> HashStruct(
      const std::string primary_type_name,
      const base::Value& data) const;
  absl::optional<std::vector<uint8_t>> EncodeData(
      const std::string& primary_type_name,
      const base::Value& data) const;
  static absl::optional<std::vector<uint8_t>> GetTypedDataMessageToSign(
      const std::vector<uint8_t>& domain_hash,
      const std::vector<uint8_t>& primary_hash);
  absl::optional<std::vector<uint8_t>> GetTypedDataPrimaryHash(
      const std::string& primary_type_name,
      const base::Value& message) const;
  absl::optional<std::vector<uint8_t>> GetTypedDataDomainHash(
      const base::Value& domain_separator) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(EthSignedTypedDataHelperUnitTest, Types);
  FRIEND_TEST_ALL_PREFIXES(EthSignedTypedDataHelperUnitTest, EncodeField);

  explicit EthSignTypedDataHelper(const base::Value& types, Version version);

  void FindAllDependencyTypes(
      base::flat_map<std::string, base::Value>* known_types,
      const std::string& anchor_type_name) const;
  std::string EncodeType(const base::Value& type,
                         const std::string& type_name) const;
  std::string EncodeTypes(const std::string& primary_type_name) const;

  absl::optional<std::vector<uint8_t>> EncodeField(
      const std::string& type,
      const base::Value& value) const;

  base::Value types_;
  Version version_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_SIGN_TYPED_DATA_HELPER_H_
