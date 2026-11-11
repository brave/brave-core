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
  std::unique_ptr<EthTransaction> tx =
      std::make_unique<EthTransaction>(*EthTransaction::FromTxData(
          mojom::TxData::New("0x1", "0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>())));
  EthTxMeta meta(eth_account_id, std::move(tx));
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
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
            meta.tx()->GetToHex());
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->value,
            Uint256ValueToHex(meta.tx()->value()));
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->data,
            meta.tx()->data());
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->base_data->chain_id,
            "0x1");
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->max_priority_fee_per_gas,
            "");
  EXPECT_EQ(ti->tx_data_union->get_eth_tx_data_1559()->max_fee_per_gas, "");
  EXPECT_EQ(meta.created_time(), ti->created_time);
  EXPECT_EQ(meta.submitted_time(), ti->submitted_time);
  EXPECT_EQ(meta.confirmed_time(), ti->confirmed_time);

  // type 1
  std::unique_ptr<Eip2930Transaction> tx1 =
      std::make_unique<Eip2930Transaction>(*Eip2930Transaction::FromTxData(
          mojom::TxData::New("0x3", "0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>())));
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
            meta1.tx()->GetToHex());
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->value,
            Uint256ValueToHex(meta1.tx()->value()));
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->data,
            meta1.tx()->data());
  auto* tx2930 = static_cast<Eip2930Transaction*>(meta1.tx());
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->base_data->chain_id,
            Uint256ValueToHex(tx2930->chain_id()));
  EXPECT_EQ(
      ti1->tx_data_union->get_eth_tx_data_1559()->max_priority_fee_per_gas, "");
  EXPECT_EQ(ti1->tx_data_union->get_eth_tx_data_1559()->max_fee_per_gas, "");

  // type2
  std::unique_ptr<Eip1559Transaction> tx2 =
      std::make_unique<Eip1559Transaction>(
          *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
              mojom::TxData::New("0x3", "0x09", "0x4a817c800", "0x5208",
                                 "0x3535353535353535353535353535353535353535",
                                 "0x0de0b6b3a7640000", std::vector<uint8_t>()),
              "0x1E", "0x32")));
  EthTxMeta meta2(eth_account_id, std::move(tx2));
  mojom::TransactionInfoPtr ti2 = meta2.ToTransactionInfo();
  EXPECT_EQ(ti2->id, meta2.id());
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
            meta2.tx()->GetToHex());
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->value,
            Uint256ValueToHex(meta2.tx()->value()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->data,
            meta2.tx()->data());
  auto* tx1559 = static_cast<Eip1559Transaction*>(meta2.tx());
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->base_data->chain_id,
            Uint256ValueToHex(tx1559->chain_id()));
  EXPECT_EQ(
      ti2->tx_data_union->get_eth_tx_data_1559()->max_priority_fee_per_gas,
      Uint256ValueToHex(tx1559->max_priority_fee_per_gas()));
  EXPECT_EQ(ti2->tx_data_union->get_eth_tx_data_1559()->max_fee_per_gas,
            Uint256ValueToHex(tx1559->max_fee_per_gas()));
}

TEST(EthTxMetaUnitTest, ToTransactionInfo_FinalRecipientTest) {
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
                mojom::TxData::New(mojom::kFilecoinEthereumMainnetChainId,
                                   "0x09", "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0de0b6b3a7640000", data),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    EXPECT_EQ(meta.ToTransactionInfo()->effective_recipient,
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
                mojom::TxData::New(mojom::kSepoliaChainId, "0x09",
                                   "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0", encoded_data),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    EXPECT_EQ(meta.ToTransactionInfo()->effective_recipient,
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
                mojom::TxData::New(mojom::kSepoliaChainId, "0x09",
                                   "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0", encoded_data),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    EXPECT_EQ(meta.ToTransactionInfo()->effective_recipient,
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
                mojom::TxData::New(mojom::kSepoliaChainId, "0x09",
                                   "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0", encoded_data),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    EXPECT_EQ(meta.ToTransactionInfo()->effective_recipient,
              "0x35353535353535353535353535353535353535bb");
  }

  // ERC1155 safe transfer final recipient
  {
    std::vector<uint8_t> encoded_data;
    std::string data;
    EXPECT_TRUE(erc1155::SafeTransferFrom(
        "0x3535353535353535353535353535353535353535",
        "0x35353535353535353535353535353535353535bb", 10, 1, &data));
    EXPECT_TRUE(PrefixedHexStringToBytes(data, &encoded_data));

    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New(mojom::kSepoliaChainId, "0x09",
                                   "0x4a817c800", "0x5208",
                                   "0x3535353535353535353535353535353535353535",
                                   "0x0", encoded_data),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    EXPECT_EQ(meta.ToTransactionInfo()->effective_recipient,
              "0x35353535353535353535353535353535353535bb");
  }

  // Just ETH transfer
  {
    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New(
                    mojom::kSepoliaChainId, "0x09", "0x4a817c800", "0x5208",
                    "0x3535353535353535353535353535353535353535",
                    "0x0de0b6b3a7640000", std::vector<uint8_t>()),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    EXPECT_EQ(meta.ToTransactionInfo()->effective_recipient,
              "0x3535353535353535353535353535353535353535");
  }
}

TEST(EthTxMetaUnitTest, ToTransactionInfo_NativeValueOnTokenCalldata) {
  // A transaction's native value always moves to `to`, whatever the calldata
  // says. Carrying a non-zero value into a non-payable token entrypoint must
  // not be described as a token transfer: doing so attributes the transaction
  // to the recipient and amount encoded in the calldata and hides the native
  // transfer.

  auto eth_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0x2f015c60e0be116b1f0cd534704db9c92118fb6a");

  constexpr char kBaseTo[] = "0xe5187d683834173d15b636ae692d6b742e57e99a";
  // TransactionInfo reports `to` and the effective recipient checksummed.
  constexpr char kBaseToChecksummed[] =
      "0xE5187d683834173D15b636aE692D6b742e57e99a";
  constexpr char kCalldataRecipient[] =
      "0x43bcbc106ecf559246637e11aff920c8128d582a";
  constexpr char kOneEth[] = "0xde0b6b3a7640000";

  auto tx_info_for = [&](const std::vector<uint8_t>& encoded_data,
                         const std::string& value) {
    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New(mojom::kSepoliaChainId, "0x09",
                                   "0x4a817c800", "0x5208", kBaseTo, value,
                                   encoded_data),
                "0x1E", "0x32")));
    return EthTxMeta(eth_account_id, std::move(tx)).ToTransactionInfo();
  };

  auto to_bytes = [](const std::string& hex) {
    std::vector<uint8_t> bytes;
    EXPECT_TRUE(PrefixedHexStringToBytes(hex, &bytes));
    return bytes;
  };

  auto expect_native_transfer = [=](const mojom::TransactionInfoPtr& tx_info,
                                    const std::string& data) {
    SCOPED_TRACE(data);

    EXPECT_EQ(tx_info->tx_type, mojom::TransactionType::Other);
    EXPECT_TRUE(tx_info->tx_params.empty());
    EXPECT_TRUE(tx_info->tx_args.empty());
    EXPECT_EQ(tx_info->effective_recipient, kBaseToChecksummed);

    // Value and calldata must be preserved.
    const auto& base_data =
        tx_info->tx_data_union->get_eth_tx_data_1559()->base_data;
    EXPECT_EQ(base_data->value, kOneEth);
    EXPECT_EQ(base_data->to, kBaseToChecksummed);
    EXPECT_EQ(base_data->data, to_bytes(data));
  };

  {
    // Prove that an ERC-20 transfer with a non-zero value is never presented as
    // a valid transfer.

    std::string data;
    ASSERT_TRUE(erc20::Transfer(kCalldataRecipient, 0, &data));

    auto tx_info = tx_info_for(to_bytes(data), kOneEth);
    expect_native_transfer(tx_info, data);
  }

  {
    // Prove that an ERC-20 transfer with a zero value is presented as a valid
    // transfer.
    std::string data;
    ASSERT_TRUE(erc20::Transfer(kCalldataRecipient, 0, &data));

    auto tx_info = tx_info_for(to_bytes(data), "0x0");

    EXPECT_EQ(tx_info->tx_type, mojom::TransactionType::ERC20Transfer);
    EXPECT_EQ(tx_info->effective_recipient, kCalldataRecipient);
  }

  {
    // Prove that an ERC-20 approve is not presented as a valid approval.

    std::string data;
    ASSERT_TRUE(erc20::Approve(kCalldataRecipient, 100, &data));

    auto tx_info = tx_info_for(to_bytes(data), kOneEth);
    expect_native_transfer(tx_info, data);
  }

  // Prove that we correctly handle the ERC721 and ERC1155 transfers.
  for (bool safe : {true, false}) {
    // ERC721 transferFrom.

    std::string data;
    ASSERT_TRUE(erc721::TransferFromOrSafeTransferFrom(
        safe, "0x2f015c60e0be116b1f0cd534704db9c92118fb6a", kCalldataRecipient,
        10, &data));

    auto tx_info = tx_info_for(to_bytes(data), kOneEth);
    expect_native_transfer(tx_info, data);
  }

  {
    // ERC721 setApprovalForAll.

    // There is no erc721::SetApprovalForAll builder, so encode by hand:
    // selector, operator address, approved.
    std::string data =
        "0xa22cb465"
        "00000000000000000000000043bcbc106ecf559246637e11aff920c8128d582a"
        "0000000000000000000000000000000000000000000000000000000000000001";

    auto tx_info = tx_info_for(to_bytes(data), "0x0");
    ASSERT_EQ(tx_info->tx_type,
              mojom::TransactionType::ERC721SetApprovalForAll);
    EXPECT_EQ(tx_info->tx_params,
              std::vector<std::string>({"address", "bool"}));
    EXPECT_EQ(tx_info->tx_args,
              std::vector<std::string>({kCalldataRecipient, "0x1"}));
    // The approval's effective recipient is the collection, not the operator.
    EXPECT_EQ(tx_info->effective_recipient, kBaseToChecksummed);

    tx_info = tx_info_for(to_bytes(data), kOneEth);
    expect_native_transfer(tx_info, data);
  }

  {
    // ERC1155 transfer.

    std::string data;
    ASSERT_TRUE(
        erc1155::SafeTransferFrom("0x2f015c60e0be116b1f0cd534704db9c92118fb6a",
                                  kCalldataRecipient, 10, 1, &data));

    EXPECT_EQ(tx_info_for(to_bytes(data), kOneEth)->tx_type,
              mojom::TransactionType::Other);
  }

  // ETHFilForwarderTransfer is payable: the value is the amount forwarded, so
  // it must keep both its classification and its calldata-derived recipient.
  {
    constexpr char kFilRecipient[] =
        "f12fopnvzwjwfu3k45sdofngoru6gpokobsbjyl2a";
    auto data =
        filforwarder::Forward(FilAddress::FromAddress(kFilRecipient)).value();

    std::unique_ptr<Eip1559Transaction> tx =
        std::make_unique<Eip1559Transaction>(
            *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                mojom::TxData::New(mojom::kFilecoinEthereumMainnetChainId,
                                   "0x09", "0x4a817c800", "0x5208", kBaseTo,
                                   kOneEth, data),
                "0x1E", "0x32")));

    EthTxMeta meta(eth_account_id, std::move(tx));
    auto tx_info = meta.ToTransactionInfo();

    EXPECT_EQ(tx_info->tx_type,
              mojom::TransactionType::ETHFilForwarderTransfer);
    EXPECT_EQ(tx_info->effective_recipient, kFilRecipient);
  }
}

}  // namespace brave_wallet
