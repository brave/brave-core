/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthTxMetaUnitTest, ToTransactionInfo) {
  const char from_address[] = "0x2f015c60e0be116b1f0cd534704db9c92118fb6a";
  auto eth_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, from_address);

  // type 0
  std::unique_ptr<EthTransaction> tx = std::make_unique<EthTransaction>(
      *EthTransaction::FromTxData(mojom::TxData::New(
          "0x09", "0x4a817c800", "0x5208",
          "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
          std::vector<uint8_t>(), false, std::nullopt)));
  EthTxMeta meta(eth_account_id, std::move(tx));
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->from_address, from_address);
  EXPECT_EQ(ti->from_account_id, meta.from());
  EXPECT_EQ(ti->tx_hash, meta.tx_hash());
  EXPECT_EQ(ti->tx_status, meta.status());

  ASSERT_TRUE(ti->tx_data_union->is_eth_tx_data_1559());
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->nonce,
            Uint256ValueToHex(meta.tx()->nonce().value()));
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->gas_price,
            Uint256ValueToHex(meta.tx()->gas_price()));
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->gas_limit,
            Uint256ValueToHex(meta.tx()->gas_limit()));
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->to,
            meta.tx()->to().ToHex());
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->value,
            Uint256ValueToHex(meta.tx()->value()));
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->data,
            meta.tx()->data());
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->chain_id, "");
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->max_priority_fee_per_gas,
            "");
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->max_fee_per_gas, "");
  EXPECT_FALSE(ti->tx_data_union->get_eth_tx_data_1559()->gas_estimation);
  EXPECT_EQ(meta.created_time().InMillisecondsSinceUnixEpoch(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().InMillisecondsSinceUnixEpoch(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().InMillisecondsSinceUnixEpoch(),
            ti->confirmed_time.InMilliseconds());

  // type 1
  std::unique_ptr<Eip2930Transaction> tx1 =
      std::make_unique<Eip2930Transaction>(*Eip2930Transaction::FromTxData(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>(),
                             false, std::nullopt),
          0x3));
  auto* access_list = tx1->access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);
  EthTxMeta meta1(eth_account_id, std::move(tx1));
  mojom::TransactionInfoPtr ti1 = meta1.ToTransactionInfo();
  EXPECT_EQ(ti1->id, meta1.id());
  EXPECT_EQ(ti1->from_address, from_address);
  EXPECT_EQ(ti1->from_account_id, meta1.from());
  EXPECT_EQ(ti1->tx_hash, meta1.tx_hash());
  EXPECT_EQ(ti1->tx_status, meta1.status());

  ASSERT_TRUE(ti1->tx_data_union->is_eth_tx_data_1559());
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->nonce,
            Uint256ValueToHex(meta1.tx()->nonce().value()));
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->gas_price,
            Uint256ValueToHex(meta1.tx()->gas_price()));
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->gas_limit,
            Uint256ValueToHex(meta1.tx()->gas_limit()));
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->to,
            meta1.tx()->to().ToHex());
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->value,
            Uint256ValueToHex(meta1.tx()->value()));
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->data,
            meta1.tx()->data());
  auto* tx2930 = static_cast<Eip2930Transaction*>(meta1.tx());
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->chain_id,
            Uint256ValueToHex(tx2930->chain_id()));
  EXPECT_EQ(
      ti1->tx_data_union->get_eth_tx_data_1559()->max_priority_fee_per_gas, "");
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->max_fee_per_gas, "");
  EXPECT_FALSE(ti1->tx_data_union->get_eth_tx_data_1559()->gas_estimation);

  // type2
  std::unique_ptr<Eip1559Transaction> tx2 =
      std::make_unique<Eip1559Transaction>(
          *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
              mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                 "0x3535353535353535353535353535353535353535",
                                 "0x0de0b6b3a7640000", std::vector<uint8_t>(),
                                 false, std::nullopt),
              "0x3", "0x1E", "0x32",
              mojom::GasEstimation1559::New(
                  "0x3b9aca00" /* Hex of 1 * 1e9 */,
                  "0xaf16b1600" /* Hex of 47 * 1e9 */,
                  "0x77359400" /* Hex of 2 * 1e9 */,
                  "0xb2d05e000" /* Hex of 48 * 1e9 */,
                  "0xb2d05e00" /* Hex of 3 * 1e9 */,
                  "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                  "0xad8075b7a" /* Hex of 46574033786 */))));
  EthTxMeta meta2(eth_account_id, std::move(tx2));
  mojom::TransactionInfoPtr ti2 = meta2.ToTransactionInfo();
  EXPECT_EQ(ti2->id, meta2.id());
  EXPECT_EQ(ti2->from_address, from_address);
  EXPECT_EQ(ti2->from_account_id, meta2.from());
  EXPECT_EQ(ti2->tx_hash, meta2.tx_hash());
  EXPECT_EQ(ti2->tx_status, meta2.status());
  ASSERT_TRUE(ti2->tx_data_union->is_eth_tx_data_1559());
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->nonce,
            Uint256ValueToHex(meta2.tx()->nonce().value()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->gas_price,
            Uint256ValueToHex(meta2.tx()->gas_price()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->gas_limit,
            Uint256ValueToHex(meta2.tx()->gas_limit()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->to,
            meta2.tx()->to().ToHex());
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->value,
            Uint256ValueToHex(meta2.tx()->value()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->data,
            meta2.tx()->data());
  auto* tx1559 = static_cast<Eip1559Transaction*>(meta2.tx());
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->chain_id,
            Uint256ValueToHex(tx1559->chain_id()));
  EXPECT_EQ(
      ti2->tx_data_union->get_eth_tx_data_1559()->max_priority_fee_per_gas,
      Uint256ValueToHex(tx1559->max_priority_fee_per_gas()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->max_fee_per_gas,
            Uint256ValueToHex(tx1559->max_fee_per_gas()));
  ASSERT_TRUE(ti2->tx_data_union->get_eth_tx_data_1559()->gas_estimation);
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()
                ->gas_estimation->slow_max_priority_fee_per_gas,
            Uint256ValueToHex(
                tx1559->gas_estimation().slow_max_priority_fee_per_gas));
  EXPECT_EQ(
      ti2->tx_data_union->get_eth_tx_data_1559()
          ->gas_estimation->avg_max_priority_fee_per_gas,
      Uint256ValueToHex(tx1559->gas_estimation().avg_max_priority_fee_per_gas));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()
                ->gas_estimation->fast_max_priority_fee_per_gas,
            Uint256ValueToHex(
                tx1559->gas_estimation().fast_max_priority_fee_per_gas));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()
                ->gas_estimation->slow_max_fee_per_gas,
            Uint256ValueToHex(tx1559->gas_estimation().slow_max_fee_per_gas));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()
                ->gas_estimation->avg_max_fee_per_gas,
            Uint256ValueToHex(tx1559->gas_estimation().avg_max_fee_per_gas));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()
                ->gas_estimation->fast_max_fee_per_gas,
            Uint256ValueToHex(tx1559->gas_estimation().fast_max_fee_per_gas));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()
                ->gas_estimation->base_fee_per_gas,
            Uint256ValueToHex(tx1559->gas_estimation().base_fee_per_gas));
}

TEST(EthTxMetaUnitTest, ToTransactionInfoFinalRecipientTest) {
  auto eth_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0x2f015c60e0be116b1f0cd534704db9c92118fb6a");

  // FilForwarder final recipient
  {
    std::vector<uint8_t> data =
        filforwarder::Forward(FilAddress::FromAddress(
                                  "f12fopnvzwjwfu3k45sdofngoru6gpokobsbjyl2a"))
            .value();

    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0de0b6b3a7640000", data, false,
                                   std::nullopt),
                mojom::kFilecoinEthereumMainnetChainId, "0x1E", "0x32",
                mojom::GasEstimation1559::New(
                    "0x3b9aca00" /* Hex of 1 * 1e9 */,
                    "0xaf16b1600" /* Hex of 47 * 1e9 */,
                    "0x77359400" /* Hex of 2 * 1e9 */,
                    "0xb2d05e000" /* Hex of 48 * 1e9 */,
                    "0xb2d05e00" /* Hex of 3 * 1e9 */,
                    "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                    "0xad8075b7a" /* Hex of 46574033786 */))));

    EthTxMeta meta(eth_account_id, std::move(tx));
    ASSERT_EQ(meta.ToTransactionInfo()->effective_recipient.value(),
              "f12fopnvzwjwfu3k45sdofngoru6gpokobsbjyl2a");
  }

  // ERC20 transfer final recipient
  {
    std::vector<uint8_t> encoded_data;
    std::string data;
    EXPECT_TRUE(erc20::Transfer("0x35353535353535353535353535353535353535bb",
                                10, &data));
    EXPECT_TRUE(PrefixedHexStringToBytes(data, &encoded_data));

    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0de0b6b3a7640000", encoded_data, false,
                                   std::nullopt),
                mojom::kSepoliaChainId, "0x1E", "0x32",
                mojom::GasEstimation1559::New(
                    "0x3b9aca00" /* Hex of 1 * 1e9 */,
                    "0xaf16b1600" /* Hex of 47 * 1e9 */,
                    "0x77359400" /* Hex of 2 * 1e9 */,
                    "0xb2d05e000" /* Hex of 48 * 1e9 */,
                    "0xb2d05e00" /* Hex of 3 * 1e9 */,
                    "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                    "0xad8075b7a" /* Hex of 46574033786 */))));

    EthTxMeta meta(eth_account_id, std::move(tx));
    ASSERT_EQ(meta.ToTransactionInfo()->effective_recipient.value(),
              "0x35353535353535353535353535353535353535bb");
  }

  // ERC721 safe transfer final recipient
  {
    std::vector<uint8_t> encoded_data;
    std::string data;
    EXPECT_TRUE(erc721::TransferFromOrSafeTransferFrom(
        true, "0x3535353535353535353535353535353535353535",
        "0x35353535353535353535353535353535353535bb", 10, &data));
    EXPECT_TRUE(PrefixedHexStringToBytes(data, &encoded_data));

    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0de0b6b3a7640000", encoded_data, false,
                                   std::nullopt),
                mojom::kSepoliaChainId, "0x1E", "0x32",
                mojom::GasEstimation1559::New(
                    "0x3b9aca00" /* Hex of 1 * 1e9 */,
                    "0xaf16b1600" /* Hex of 47 * 1e9 */,
                    "0x77359400" /* Hex of 2 * 1e9 */,
                    "0xb2d05e000" /* Hex of 48 * 1e9 */,
                    "0xb2d05e00" /* Hex of 3 * 1e9 */,
                    "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                    "0xad8075b7a" /* Hex of 46574033786 */))));

    EthTxMeta meta(eth_account_id, std::move(tx));
    ASSERT_EQ(meta.ToTransactionInfo()->effective_recipient.value(),
              "0x35353535353535353535353535353535353535bb");
  }

  // ERC721 unsafe transfer final recipient
  {
    std::vector<uint8_t> encoded_data;
    std::string data;
    EXPECT_TRUE(erc721::TransferFromOrSafeTransferFrom(
        false, "0x3535353535353535353535353535353535353535",
        "0x35353535353535353535353535353535353535bb", 10, &data));
    EXPECT_TRUE(PrefixedHexStringToBytes(data, &encoded_data));

    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0de0b6b3a7640000", encoded_data, false,
                                   std::nullopt),
                mojom::kSepoliaChainId, "0x1E", "0x32",
                mojom::GasEstimation1559::New(
                    "0x3b9aca00" /* Hex of 1 * 1e9 */,
                    "0xaf16b1600" /* Hex of 47 * 1e9 */,
                    "0x77359400" /* Hex of 2 * 1e9 */,
                    "0xb2d05e000" /* Hex of 48 * 1e9 */,
                    "0xb2d05e00" /* Hex of 3 * 1e9 */,
                    "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                    "0xad8075b7a" /* Hex of 46574033786 */))));

    EthTxMeta meta(eth_account_id, std::move(tx));
    ASSERT_EQ(meta.ToTransactionInfo()->effective_recipient.value(),
              "0x35353535353535353535353535353535353535bb");
  }

  // Just ETH transfer
  {
    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0de0b6b3a7640000", std::vector<uint8_t>(),
                                   false, std::nullopt),
                mojom::kSepoliaChainId, "0x1E", "0x32",
                mojom::GasEstimation1559::New(
                    "0x3b9aca00" /* Hex of 1 * 1e9 */,
                    "0xaf16b1600" /* Hex of 47 * 1e9 */,
                    "0x77359400" /* Hex of 2 * 1e9 */,
                    "0xb2d05e000" /* Hex of 48 * 1e9 */,
                    "0xb2d05e00" /* Hex of 3 * 1e9 */,
                    "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                    "0xad8075b7a" /* Hex of 46574033786 */))));

    EthTxMeta meta(eth_account_id, std::move(tx));
    ASSERT_EQ(meta.ToTransactionInfo()->effective_recipient.value(),
              "0x3535353535353535353535353535353535353535");
  }
}

}  // namespace brave_wallet
