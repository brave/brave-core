/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/hd_key.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class FilecoinKeyring : public HDKeyring {
 public:
  FilecoinKeyring();
  ~FilecoinKeyring() override;

  Type type() const override;

  std::string ImportFilecoinSECP256K1Account(
      const std::vector<uint8_t>& input_key,
      const std::string& network);
  std::string ImportFilecoinBLSAccount(const std::vector<uint8_t>& private_key,
                                       const std::vector<uint8_t>& public_key,
                                       const std::string& network);
  void ImportFilecoinAccount(const std::vector<uint8_t>& input_key,
                             const std::string& address);
  std::string GetAddressInternal(const HDKey* hd_key) const override;

 protected:
  std::string CreateAddressWithProtocol(const std::vector<uint8_t>& payload,
                                        int protocol) const;
  std::unique_ptr<HDKey> root_;
  std::unique_ptr<HDKey> master_key_;

 private:
  friend FilecoinKeyring;
  FilecoinKeyring(const FilecoinKeyring&) = delete;
  FilecoinKeyring& operator=(const FilecoinKeyring&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_
