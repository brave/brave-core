/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc_responses.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

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

bool ReadU128(base::SpanReader<const uint8_t>& reader, mojom::uint128Ptr& out) {
  auto val = mojom::uint128::New(0, 0);

  if (!reader.ReadU64LittleEndian(val->low)) {
    return false;
  }

  if (!reader.ReadU64LittleEndian(val->high)) {
    return false;
  }

  out = std::move(val);
  return true;
}

mojom::PolkadotAccountInfoPtr ParseAccountInfoAsHex(
    base::span<const uint8_t, 80> bytes) {
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

  if (!ReadU128(reader, account->data->free)) {
    return nullptr;
  }

  if (!ReadU128(reader, account->data->reserved)) {
    return nullptr;
  }

  if (!ReadU128(reader, account->data->frozen)) {
    return nullptr;
  }

  if (!ReadU128(reader, account->data->flags)) {
    return nullptr;
  }

  DCHECK_EQ(reader.remaining(), 0u);
  return account;
}

mojom::PolkadotAccountInfoPtr MakeDefaultAccount() {
  // Default value defined here:
  // https://github.com/polkadot-js/api/blob/1c4c7c72e281da328084ae821218efb9fe7120ac/packages/types-support/src/metadata/v16/substrate-json.json#L23

  auto account = mojom::PolkadotAccountInfo::New();
  account->data = mojom::PolkadotAccountBalance::New();

  account->nonce = 0;
  account->consumers = 0;
  account->providers = 0;
  account->sufficients = 0;

  account->data->free = mojom::uint128::New(0, 0);
  account->data->reserved = mojom::uint128::New(0, 0);
  account->data->frozen = mojom::uint128::New(0, 0);
  account->data->flags = mojom::uint128::New(0x8000000000000000, 0);
  return account;
}

mojom::PolkadotAccountInfoPtr ParseAccountInfoFromJson(
    const std::optional<
        std::vector<polkadot_substrate_rpc_responses::AccountInfo>>& result) {
  // See `"id": 3` for more information:
  // https://raw.githubusercontent.com/polkadot-js/api/refs/heads/master/packages/types-support/src/metadata/v16/substrate-types.json

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

  auto* str = (*list)[1].GetIfString();
  if (!str) {
    return MakeDefaultAccount();
  }

  std::string_view sv = *str;

  auto hex_bytes = PrefixedHexStringToFixed<80>(sv);
  if (!hex_bytes) {
    return nullptr;
  }

  auto account = ParseAccountInfoAsHex(*hex_bytes);
  if (account) {
    return account;
  }

  return nullptr;
}

template <class RpcResponse>
base::expected<RpcResponse, std::string> HandleRpcCall(
    const APIRequestResult& api_result) {
  if (!api_result.Is2XXResponseCode()) {
    return base::unexpected(WalletInternalErrorMessage());
  }

  auto res = RpcResponse::FromValue(api_result.value_body());

  if (!res) {
    return base::unexpected(WalletParsingErrorMessage());
  }

  if (res->error) {
    if (res->error->message) {
      return base::unexpected(res->error->message.value());
    }
    return base::unexpected(WalletInternalErrorMessage());
  }

  return base::ok(std::move(*res));
}

std::optional<PolkadotBlockHeader> ParseChainHeaderFromHex(
    const polkadot_substrate_rpc_responses::PolkadotChainHeader& res) {
  PolkadotBlockHeader header;

  auto parent_hash =
      PrefixedHexStringToFixed<kPolkadotBlockHashSize>(res.result->parent_hash);
  if (!parent_hash) {
    return std::nullopt;
  }

  header.parent_hash = *parent_hash;

  std::string_view block_number = res.result->number;
  if (!base::HexStringToUInt(block_number, &header.number)) {
    return std::nullopt;
  }

  return header;
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

void PolkadotSubstrateRpc::GetChainName(std::string_view chain_id,
                                        GetChainNameCallback callback) {
  auto url = GetNetworkURL(chain_id);

  auto payload =
      base::WriteJson(MakeRpcRequestJson("system_chain", base::ListValue()));
  CHECK(payload);

  api_request_helper_.Request(
      net::HttpRequestHeaders::kPostMethod, url, *payload, "application/json",
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
    std::string_view chain_id,
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
  constexpr char const kSystemPallet[] = "26AA394EEA5630E07C48AE0C9558CEF7";
  constexpr char const kAccount[] = "B99D880EC681799C0CF30E8886371DA9";

  auto checksum = Blake2bHash<16>({pubkey});

  auto rpc_cmd =
      base::StrCat({"0x", kSystemPallet, kAccount, base::HexEncode(checksum),
                    base::HexEncode(pubkey)});

  auto payload = base::WriteJson(MakeRpcRequestJson(
      "state_queryStorageAt",
      base::Value::List().Append(
          base::Value(base::Value::List().Append(std::move(rpc_cmd))))));

  CHECK(payload);

  auto url = GetNetworkURL(chain_id);

  api_request_helper_.Request(
      net::HttpRequestHeaders::kPostMethod, url, *payload, "application/json",
      base::BindOnce(&PolkadotSubstrateRpc::OnGetAccountBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetAccountBalance(
    GetAccountBalanceCallback callback,
    APIRequestResult api_result) {
  auto res = HandleRpcCall<
      polkadot_substrate_rpc_responses::PolkadotAccountBalanceResponse>(
      api_result);

  if (!res.has_value()) {
    return std::move(callback).Run(nullptr, res.error());
  }

  auto account = ParseAccountInfoFromJson(res->result);
  if (account) {
    return std::move(callback).Run(std::move(account), std::nullopt);
  }

  return std::move(callback).Run(nullptr, WalletParsingErrorMessage());
}

void PolkadotSubstrateRpc::GetFinalizedHead(std::string_view chain_id,
                                            GetFinalizedHeadCallback callback) {
  auto url = GetNetworkURL(chain_id);

  auto payload = base::WriteJson(
      MakeRpcRequestJson("chain_getFinalizedHead", base::ListValue()));
  CHECK(payload);

  api_request_helper_.Request(
      net::HttpRequestHeaders::kPostMethod, url, *payload, "application/json",
      base::BindOnce(&PolkadotSubstrateRpc::OnGetFinalizedHead,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetFinalizedHead(GetFinalizedHeadCallback callback,
                                              APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotFinalizedHead>(
          api_result);

  if (!res.has_value()) {
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    return std::move(callback).Run(std::nullopt, std::nullopt);
  }

  auto block_hash =
      PrefixedHexStringToFixed<kPolkadotBlockHashSize>(*res->result);
  if (!block_hash) {
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  return std::move(callback).Run(block_hash, std::nullopt);
}

void PolkadotSubstrateRpc::GetBlockHeader(
    std::string_view chain_id,
    std::optional<base::span<uint8_t, kPolkadotBlockHashSize>> blockhash,
    GetBlockHeaderCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;

  if (blockhash) {
    params.Append(base::HexEncode(*blockhash));
  }

  auto payload =
      base::WriteJson(MakeRpcRequestJson("chain_getHeader", std::move(params)));
  CHECK(payload);

  api_request_helper_.Request(
      net::HttpRequestHeaders::kPostMethod, url, *payload, "application/json",
      base::BindOnce(&PolkadotSubstrateRpc::OnGetBlockHeader,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetBlockHeader(GetBlockHeaderCallback callback,
                                            APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotChainHeader>(
          api_result);

  if (!res.has_value()) {
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    return std::move(callback).Run(std::nullopt, std::nullopt);
  }

  auto header = ParseChainHeaderFromHex(*res);
  if (!header) {
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  std::move(callback).Run(std::move(*header), std::nullopt);
}

GURL PolkadotSubstrateRpc::GetNetworkURL(std::string_view chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::DOT);
}

}  // namespace brave_wallet
