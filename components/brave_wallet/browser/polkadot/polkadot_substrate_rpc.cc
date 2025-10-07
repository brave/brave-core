/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

using APIRequestResult = api_request_helper::APIRequestResult;

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("polkadot_substrate_rpc", R"(
      semantics {
        sender: "Polkadot Substrate RPC"
        description:
          "This service is used to communicate with Polkadot Substrate nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Polkadot Substrate JSON RPC response bodies."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

mojom::PolkadotAccountInfoPtr ParseAccountInfoAsHex(std::string_view sv) {
  // leading 0x, 160 hex chars worth of data
  if (sv.size() != 162) {
    return nullptr;
  }

  if (!sv.starts_with("0x")) {
    return nullptr;
  }

  std::vector<uint8_t> bytes;
  sv.remove_prefix(2);  // remove leading 0x
  if (!base::HexStringToBytes(sv, &bytes)) {
    return nullptr;
  }

  base::SpanReader<const uint8_t> reader(bytes);
  auto account = mojom::PolkadotAccountInfo::New();

  if (!reader.ReadU32LittleEndian(account->nonce)) {
    return nullptr;
  }

  if (!reader.ReadU32LittleEndian(account->consumers)) {
    return nullptr;
  }

  if (!reader.ReadU32LittleEndian(account->providers)) {
    return nullptr;
  }

  if (!reader.ReadU32LittleEndian(account->sufficients)) {
    return nullptr;
  }

  account->data = mojom::PolkadotAccountBalance::New();

  uint64_t low = 0, high = 0;
  if (!reader.ReadU64LittleEndian(low)) {
    return nullptr;
  }

  if (!reader.ReadU64LittleEndian(high)) {
    return nullptr;
  }

  account->data->free = mojom::uint128Ptr(std::in_place, high, low);

  if (!reader.ReadU64LittleEndian(low)) {
    return nullptr;
  }

  if (!reader.ReadU64LittleEndian(high)) {
    return nullptr;
  }

  account->data->reserved = mojom::uint128Ptr(std::in_place, high, low);

  if (!reader.ReadU64LittleEndian(low)) {
    return nullptr;
  }

  if (!reader.ReadU64LittleEndian(high)) {
    return nullptr;
  }

  account->data->frozen = mojom::uint128Ptr(std::in_place, high, low);

  if (!reader.ReadU64LittleEndian(low)) {
    return nullptr;
  }

  if (!reader.ReadU64LittleEndian(high)) {
    return nullptr;
  }

  account->data->flags = mojom::uint128Ptr(std::in_place, high, low);

  return account;
}

mojom::PolkadotAccountInfoPtr ParseAccountInfoFromJson(
    const std::optional<
        std::vector<polkadot_substrate_rpc_responses::AccountInfo>>& result) {
  /*

  The shape of the resturned JSON is:

  {
    ...,
    "result": [
      {
        "block": "<block hash>",
        "changes": [["<storage key>", "<account info>" | null], ...]
      },
      ...
    ]
  }

  */

  if (!result) {
    return nullptr;
  }

  auto& accounts = *result;
  if (accounts.size() != 1) {
    return nullptr;
  }

  auto& changes = accounts[0].changes;
  if (changes.empty()) {
    return nullptr;
  }

  auto* list = changes[0].GetIfList();
  if (!list || list->size() != 2) {
    return nullptr;
  }

  constexpr const char kFallback[] =
      "0x0000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000080";

  std::string_view sv;
  if (auto* str = (*list)[1].GetIfString()) {
    sv = *str;
  } else {
    sv = kFallback;
  }

  auto account = ParseAccountInfoAsHex(sv);
  if (account) {
    return account;
  }

  return nullptr;
}

}  // namespace

PolkadotSubstrateRpc::PolkadotSubstrateRpc(
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : network_manager_(network_manager),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

PolkadotSubstrateRpc::~PolkadotSubstrateRpc() = default;

base::DictValue PolkadotSubstrateRpc::MakeRpcRequestJson(
    std::string_view method,
    base::ListValue params) {
  base::DictValue req;
  req.Set("id", 1);
  req.Set("jsonrpc", "2.0");
  req.Set("method", method);
  req.Set("params", std::move(params));
  return req;
}

void PolkadotSubstrateRpc::GetChainName(const std::string& chain_id,
                                        GetChainNameCallback callback) {
  auto url = GetNetworkURL(chain_id);

  std::string method = net::HttpRequestHeaders::kPostMethod;
  std::string payload_content_type = "application/json";
  auto payload =
      base::WriteJson(MakeRpcRequestJson("system_chain", base::ListValue()));

  api_request_helper_.Request(
      method, url, *payload, payload_content_type,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetChainName,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetChainName(GetChainNameCallback callback,
                                          APIRequestResult api_result) {
  if (!api_result.Is2XXResponseCode()) {
    return std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
  }

  auto res =
      polkadot_substrate_rpc_responses::PolkadotSystemChainResponse::FromValue(
          api_result.value_body());

  if (!res) {
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  if (res->error) {
    if (res->error->message) {
      return std::move(callback).Run(std::nullopt, res->error->message.value());
    }
    return std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
  }

  if (res->result) {
    return std::move(callback).Run(*res->result, std::nullopt);
  }

  return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
}

void PolkadotSubstrateRpc::GetAccountBalance(
    const std::string& chain_id,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> pubkey,
    GetAccountBalanceCallback callback) {
  // xxhash("System") | xxhash("Account")
  //
  // https://github.com/polkadot-js/common/blob/047840319ef3f758880cc112b987888b8b2749d0/packages/util-crypto/src/xxhash/asU8a.ts#L24
  // https://github.com/paritytech/polkadot-sdk/blob/cf439301b2a9571e5fcb04e4550167a878187182/substrate/primitives/crypto/hashing/src/lib.rs#L77-L82
  //
  // pub fn twox_128_into(data: &[u8], dest: &mut [u8; 16]) {
  //     let r0 = twox_hash::XxHash::with_seed(0).chain_update(data).finish();
  //     let r1 = twox_hash::XxHash::with_seed(1).chain_update(data).finish();
  //     LittleEndian::write_u64(&mut dest[0..8], r0);
  //     LittleEndian::write_u64(&mut dest[8..16], r1);
  // }
  constexpr char const kStoragePallet[] = "26AA394EEA5630E07C48AE0C9558CEF7";
  constexpr char const kAccount[] = "B99D880EC681799C0CF30E8886371DA9";

  auto checksum = Blake2bHash<16>({pubkey});

  auto rpc_cmd =
      base::StrCat({"0x", kStoragePallet, kAccount, base::HexEncode(checksum),
                    base::HexEncode(pubkey)});

  auto payload = base::WriteJson(MakeRpcRequestJson(
      "state_queryStorageAt",
      base::Value::List().Append(
          base::Value(base::Value::List().Append(std::move(rpc_cmd))))));

  CHECK(payload);

  auto url = GetNetworkURL(chain_id);

  std::string method = net::HttpRequestHeaders::kPostMethod;
  std::string payload_content_type = "application/json";

  api_request_helper_.Request(
      method, url, *payload, payload_content_type,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetAccountBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetAccountBalance(
    GetAccountBalanceCallback callback,
    APIRequestResult api_result) {
  auto res = polkadot_substrate_rpc_responses::PolkadotAccountBalanceResponse::
      FromValue(api_result.value_body());

  if (!res) {
    return std::move(callback).Run(nullptr, WalletParsingErrorMessage());
  }

  if (res->error) {
    if (res->error->message) {
      return std::move(callback).Run(nullptr, res->error->message.value());
    }
    return std::move(callback).Run(nullptr, WalletInternalErrorMessage());
  }

  auto account = ParseAccountInfoFromJson(res->result);
  if (account) {
    return std::move(callback).Run(std::move(account), std::nullopt);
  }

  return std::move(callback).Run(nullptr, WalletParsingErrorMessage());
}

GURL PolkadotSubstrateRpc::GetNetworkURL(const std::string& chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::DOT);
}

}  // namespace brave_wallet
