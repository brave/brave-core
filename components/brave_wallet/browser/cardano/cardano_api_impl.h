/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_API_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_API_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;

class CardanoApiImpl final : public mojom::CardanoApi {
 public:
  CardanoApiImpl(BraveWalletService& brave_wallet_service,
                 std::unique_ptr<BraveWalletProviderDelegate> delegate,
                 mojom::AccountIdPtr selected_account);
  ~CardanoApiImpl() override;

  void GetNetworkId(GetNetworkIdCallback callback) override;
  void GetUsedAddresses(GetUsedAddressesCallback callback) override;
  void GetUnusedAddresses(GetUnusedAddressesCallback callback) override;
  void GetChangeAddress(GetChangeAddressCallback callback) override;
  void GetRewardAddresses(GetRewardAddressesCallback callback) override;
  void GetBalance(GetBalanceCallback callback) override;
  void GetUtxos(const std::optional<std::string>& amount_cbor,
                mojom::CardanoProviderPaginationPtr paginate,
                GetUtxosCallback callback) override;
  void SignTx(const std::string& tx_cbor,
              bool partial_sign,
              SignTxCallback callback) override;
  void SubmitTx(const std::string& signed_tx_cbor,
                SubmitTxCallback callback) override;
  void SignData(const std::string& address,
                const std::string& payload_hex,
                SignDataCallback callback) override;
  void GetCollateral(const std::string& amount,
                     GetCollateralCallback callback) override;

 private:
  friend class CardanoApiImplTest;

  void OnSignMessageRequestProcessed(const mojom::AccountIdPtr& account_id,
                                     const mojom::CardanoKeyIdPtr& key_id,
                                     const std::vector<uint8_t>& message,
                                     SignDataCallback callback,
                                     bool approved,
                                     mojom::EthereumSignatureBytesPtr signature,
                                     const std::optional<std::string>& error);

  void OnGetBalance(GetBalanceCallback callback,
                    mojom::CardanoBalancePtr balance,
                    const std::optional<std::string>& error);
  void OnGetUtxos(
      std::optional<uint64_t> amount,
      mojom::CardanoProviderPaginationPtr paginate,
      GetUtxosCallback callback,
      base::expected<cardano_rpc::UnspentOutputs, std::string> all_utxos);
  void OnSubmitTx(SubmitTxCallback callback,
                  base::expected<std::string, std::string> txid);
  void OnGetUtxosForSignTx(
      CardanoTxDecoder::RestoredTransaction tx,
      bool partial_sign,
      SignTxCallback callback,
      base::expected<cardano_rpc::UnspentOutputs, std::string>);

  mojom::CardanoProviderErrorBundlePtr CheckSelectedAccountValid();

  void OnAddUnapprovedTransaction(SubmitTxCallback callback,
                                  bool success,
                                  const std::string& tx_meta_id,
                                  const std::string& error_message);

  bool InsertKnowInputAddresses(
      const cardano_rpc::UnspentOutputs& utxos,
      CardanoTxDecoder::RestoredTransaction& transaction,
      bool partial_sign);

  void OnSignTransactionRequestProcessed(
      CardanoTxDecoder::RestoredTransaction tx,
      SignTxCallback callback,
      bool approved,
      const std::optional<std::string>& error);

  mojom::SignCardanoTransactionRequestPtr FromRestoredTransaction(
      const CardanoTxDecoder::RestoredTransaction& tx);

  BraveWalletProviderDelegate* delegate();

  raw_ref<BraveWalletService> brave_wallet_service_;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  mojom::AccountIdPtr selected_account_;

  base::WeakPtrFactory<CardanoApiImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_API_IMPL_H_
