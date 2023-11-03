/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class FilTransaction;

class FilecoinKeyring : public HDKeyring {
 public:
  explicit FilecoinKeyring(const std::string& chain_id);
  ~FilecoinKeyring() override;
  FilecoinKeyring(const FilecoinKeyring&) = delete;
  FilecoinKeyring& operator=(const FilecoinKeyring&) = delete;
  static bool DecodeImportPayload(const std::string& payload_hex,
                                  std::vector<uint8_t>* private_key_out,
                                  mojom::FilecoinAddressProtocol* protocol_out);

  using ImportFilecoinAccountCallback =
      base::OnceCallback<void(absl::optional<std::string>)>;
  void ImportFilecoinAccount(const std::vector<uint8_t>& private_key,
                             mojom::FilecoinAddressProtocol protocol,
                             ImportFilecoinAccountCallback callback);

  void RestoreFilecoinAccount(const std::vector<uint8_t>& input_key,
                              const std::string& address);

  using SignTransactionCallback =
      base::OnceCallback<void(absl::optional<std::string>)>;
  void SignTransaction(const std::string& address,
                       const FilTransaction* tx,
                       SignTransactionCallback callback);
  std::string EncodePrivateKeyForExport(const std::string& address) override;

 private:
  void OnGetPublicKey(ImportFilecoinAccountCallback callback,
                      mojom::FilecoinAddressProtocol protocol,
                      std::unique_ptr<HDKey> hd_key,
                      const absl::optional<std::vector<uint8_t>>& public_key);

  std::string GetAddressInternal(HDKeyBase* hd_key_base) const override;
  std::unique_ptr<HDKeyBase> DeriveAccount(uint32_t index) const override;
  std::string network_;

  base::WeakPtrFactory<FilecoinKeyring> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_
