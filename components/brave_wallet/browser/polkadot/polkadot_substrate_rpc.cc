/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/bit_cast.h"
#include "base/check.h"
#include "base/containers/extend.h"
#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/numerics/checked_math.h"
#include "base/strings/strcat.h"  // IWYU pragma: export
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc_responses.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "net/http/http_request_headers.h"

namespace brave_wallet {

using APIRequestResult = api_request_helper::APIRequestResult;

namespace {

// Account info comes to us through the wire as 160 hex digits.
inline constexpr size_t kPolkadotAccountInfoSize = 80;

// AssetAccount info comes to us through the wire as 36 hex digits.
inline constexpr size_t kPolkadotAssetAccountInfoSize = 18;

base::flat_map<std::string, std::string> MakePolkadotRpcHeaders(
    const GURL& request_url) {
  return IsEndpointUsingBraveWalletProxy(request_url)
             ? MakeBraveServicesKeyHeaders()
             : base::flat_map<std::string, std::string>();
}

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
    base::span<const uint8_t, kPolkadotAccountInfoSize> bytes) {
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

mojom::PolkadotAssetAccountInfoPtr ParseAssetAccountInfoAsHex(
    base::span<const uint8_t, kPolkadotAssetAccountInfoSize> bytes) {
  // The AssetBalance struct is described here:
  // https://github.com/paritytech/polkadot-sdk/blob/81e6d5ac17544a9b11a177e5e16c8ca5c3887a6f/substrate/frame/assets/src/types.rs#L175-L188
  //
  // Balance = u128
  // Status = u8
  // Reason = u8
  // Extra = ()

  base::SpanReader<const uint8_t> reader(bytes);
  auto asset_account = mojom::PolkadotAssetAccountInfo::New();

  if (!ReadU128(reader, asset_account->balance)) {
    return nullptr;
  }

  uint8_t status = 0;
  if (!reader.ReadU8LittleEndian(status)) {
    return nullptr;
  }

  uint8_t reason = 0;
  if (!reader.ReadU8LittleEndian(reason)) {
    return nullptr;
  }

  DCHECK_EQ(reader.remaining(), 0u);
  return asset_account;
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

  std::array<uint8_t, kPolkadotAccountInfoSize> hex_bytes = {};
  if (!PrefixedHexStringToFixed(sv, hex_bytes)) {
    return nullptr;
  }

  auto account = ParseAccountInfoAsHex(hex_bytes);
  if (account) {
    return account;
  }

  return nullptr;
}

std::optional<std::vector<mojom::PolkadotAssetAccountInfoPtr>>
ParseAssetAccountInfoFromJson(
    const std::optional<
        std::vector<polkadot_substrate_rpc_responses::AccountInfo>>& result) {
  if (!result) {
    return std::nullopt;
  }

  auto& accounts = *result;
  if (accounts.size() != 1) {
    return std::nullopt;
  }

  auto& changes = accounts[0].changes;
  if (changes.empty()) {
    return std::nullopt;
  }

  std::vector<mojom::PolkadotAssetAccountInfoPtr> asset_accounts;

  // "changes" in the JSON is: [ [<StorageKey>, <AssetAccount> | null], ...].
  for (const auto& change : changes) {
    auto* list = change.GetIfList();
    if (!list || list->size() != 2) {
      return std::nullopt;
    }

    const auto* str = (*list)[1].GetIfString();
    if (!str) {
      auto p = mojom::PolkadotAssetAccountInfo::New();
      p->balance = Uint128ToMojom(uint128_t{0});
      asset_accounts.push_back(std::move(p));
      continue;
    }

    std::string_view sv = *str;
    std::array<uint8_t, kPolkadotAssetAccountInfoSize> hex_bytes = {};
    if (!PrefixedHexStringToFixed(sv, hex_bytes)) {
      return std::nullopt;
    }

    auto account_asset = ParseAssetAccountInfoAsHex(hex_bytes);
    if (!account_asset) {
      return std::nullopt;
    }

    asset_accounts.push_back(std::move(account_asset));
  }

  return asset_accounts;
}

std::optional<std::vector<mojom::BlockchainTokenPtr>>
ParseAssetMetadataInfoFromJson(
    std::string_view chain_id,
    base::span<const uint32_t> asset_ids,
    const std::optional<
        std::vector<polkadot_substrate_rpc_responses::AccountInfo>>& result) {
  if (!result) {
    return std::nullopt;
  }

  auto& accounts = *result;
  if (accounts.size() != 1) {
    return std::nullopt;
  }

  auto& changes = accounts[0].changes;
  if (changes.empty()) {
    return std::nullopt;
  }

  // `changes` is returned in the same order as the storage keys we built from
  // `asset_ids` (one entry per key), so the two are positionally aligned.
  if (changes.size() != asset_ids.size()) {
    return std::nullopt;
  }

  std::vector<mojom::BlockchainTokenPtr> tokens;

  // "changes" in the JSON is: [ [<StorageKey>, <AssetAccount> | null], ...].
  for (size_t i = 0; i < changes.size(); ++i) {
    const auto& change = changes[i];
    uint32_t asset_id = asset_ids[i];

    auto* list = change.GetIfList();
    if (!list || list->size() != 2) {
      return std::nullopt;
    }

    const auto* str = (*list)[1].GetIfString();
    if (!str) {
      return std::nullopt;
    }

    std::string_view sv = *str;
    std::vector<uint8_t> hex_bytes;
    if (!PrefixedHexStringToBytes(sv, &hex_bytes)) {
      return std::nullopt;
    }

    auto asset_metadata =
        decode_asset_metadata(::rust::Slice<const uint8_t>(hex_bytes));

    if (!asset_metadata->is_ok()) {
      return std::nullopt;
    }

    auto token_info = asset_metadata->unwrap();

    auto token = mojom::BlockchainToken::New();
    token->chain_id = chain_id;
    token->coin = mojom::CoinType::DOT;
    token->contract_address = base::NumberToString(asset_id);
    token->name = std::string(base::as_string_view(token_info->name));
    token->symbol = std::string(base::as_string_view(token_info->symbol));
    token->decimals = token_info->decimals;  // Placeholder, see TODO above.
    token->visible = true;
    token->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

    tokens.push_back(std::move(token));

    LOG(INFO) << std::string(base::as_string_view(token_info->name));
    LOG(INFO) << std::string(base::as_string_view(token_info->symbol));
    LOG(INFO) << static_cast<int>(token_info->decimals);
  }

  return tokens;
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
    const polkadot_substrate_rpc_responses::ChainHeader& res) {
  PolkadotBlockHeader header;

  if (!PrefixedHexStringToFixed(res.parent_hash, header.parent_hash)) {
    return std::nullopt;
  }

  if (!base::HexStringToUInt(res.number, &header.block_number)) {
    return std::nullopt;
  }

  if (!PrefixedHexStringToFixed(res.state_root, header.state_root)) {
    return std::nullopt;
  }

  if (!PrefixedHexStringToFixed(res.extrinsics_root, header.extrinsics_root)) {
    return std::nullopt;
  }

  // We need this for hashing the block header.
  // If we receive more than u32::MAX logs, it's safe to say we're getting bad
  // data from the remote.
  auto num_logs = base::CheckedNumeric<uint32_t>(res.digest.logs.size());
  if (!num_logs.IsValid()) {
    return std::nullopt;
  }

  auto enc_num_logs = compact_scale_encode_u32(num_logs.ValueOrDie());

  base::Extend(header.encoded_logs, enc_num_logs);
  for (const auto& log_str : res.digest.logs) {
    std::vector<uint8_t> log;
    if (!PrefixedHexStringToBytes(log_str, &log)) {
      return std::nullopt;
    }
    base::Extend(header.encoded_logs, log);
  }

  return header;
}

std::optional<std::vector<std::string>> ParseExtrinsics(
    polkadot_substrate_rpc_responses::ChainBlockData& res) {
  for (const auto& extrinsic : res.extrinsics) {
    if (!IsValidHexString(extrinsic)) {
      return std::nullopt;
    }
  }
  return std::move(res.extrinsics);
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

  MakePostRequestInternal(
      url, *payload,
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
      base::ListValue().Append(
          base::Value(base::ListValue().Append(std::move(rpc_cmd))))));

  CHECK(payload);

  auto url = GetNetworkURL(chain_id);

  MakePostRequestInternal(
      url, *payload,
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

void PolkadotSubstrateRpc::GetAssetAccountBalances(
    std::string_view chain_id,
    base::span<const uint32_t> asset_ids,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> pubkey,
    GetAssetAccountBalancesCallback callback) {
  // The shape of the storage key is described here:
  // https://github.com/paritytech/polkadot-sdk/blob/81e6d5ac17544a9b11a177e5e16c8ca5c3887a6f/substrate/frame/assets/src/lib.rs#L445-L454

  static constexpr const char kAssetsPallet[] =
      "682a59d51ab9e48a8c8cc418ff9708d2";
  static constexpr const char kAccountQuery[] =
      "b99d880ec681799c0cf30e8886371da9";

  auto pubkey_checksum = base::HexEncodeLower(Blake2bHash<16>({pubkey}));
  auto pubkey_hex = base::HexEncodeLower(pubkey);

  base::ListValue storage_keys;
  storage_keys.reserve(asset_ids.size());

  for (auto asset_id : asset_ids) {
    auto le_asset_id = base::byte_span_from_ref(asset_id);

    auto asset_id_checksum =
        base::HexEncodeLower(Blake2bHash<16>({le_asset_id}));

    storage_keys.Append(base::StrCat(
        {"0x", kAssetsPallet, kAccountQuery, asset_id_checksum,
         base::HexEncodeLower(le_asset_id), pubkey_checksum, pubkey_hex}));
  }

  auto payload = base::WriteJson(MakeRpcRequestJson(
      "state_queryStorageAt",
      base::ListValue().Append(base::Value(std::move(storage_keys)))));

  CHECK(payload);

  auto url = GetNetworkURL(chain_id);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetAssetAccountBalances,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetAssetAccountBalances(
    GetAssetAccountBalancesCallback callback,
    APIRequestResult api_result) {
  auto res = HandleRpcCall<
      polkadot_substrate_rpc_responses::PolkadotAssetAccountBalanceResponse>(
      api_result);

  if (!res.has_value()) {
    return std::move(callback).Run({}, res.error());
  }

  auto asset_accounts = ParseAssetAccountInfoFromJson(res->result);
  if (!asset_accounts) {
    return std::move(callback).Run({}, WalletParsingErrorMessage());
  }

  return std::move(callback).Run(std::move(asset_accounts.value()), {});
}

void PolkadotSubstrateRpc::GetAssetMetadata(
    std::string_view chain_id,
    base::span<const uint32_t> asset_ids,
    GetAssetMetadataCallback callback) {
  // The shape of the storage key is described here:
  // https://github.com/paritytech/polkadot-sdk/blob/fc2028840cf2a2dc2d44f41f099492797269c63d/substrate/frame/assets/src/lib.rs#L470-L478

  static constexpr const char kAssetsPallet[] =
      "682a59d51ab9e48a8c8cc418ff9708d2";
  static constexpr const char kMetadataQuery[] =
      "b5f3822e35ca2f31ce3526eab1363fd2";

  base::ListValue storage_keys;
  storage_keys.reserve(asset_ids.size());

  for (auto asset_id : asset_ids) {
    auto le_asset_id = base::byte_span_from_ref(asset_id);

    auto asset_id_checksum =
        base::HexEncodeLower(Blake2bHash<16>({le_asset_id}));

    storage_keys.Append(
        base::StrCat({"0x", kAssetsPallet, kMetadataQuery, asset_id_checksum,
                      base::HexEncodeLower(le_asset_id)}));
  }

  auto payload = base::WriteJson(MakeRpcRequestJson(
      "state_queryStorageAt",
      base::ListValue().Append(base::Value(std::move(storage_keys)))));

  CHECK(payload);

  LOG(INFO) << "request:";
  LOG(INFO) << *payload;

  auto url = GetNetworkURL(chain_id);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetAssetMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::string(chain_id),
                     base::ToVector(asset_ids), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetAssetMetadata(std::string chain_id,
                                              std::vector<uint32_t> asset_ids,
                                              GetAssetMetadataCallback callback,
                                              APIRequestResult api_result) {
  auto res = HandleRpcCall<
      polkadot_substrate_rpc_responses::PolkadotAssetMetadataResponse>(
      api_result);

  if (!res.has_value()) {
    return std::move(callback).Run(std::nullopt, res.error());
  }

  auto tokens =
      ParseAssetMetadataInfoFromJson(chain_id, asset_ids, res->result);
  if (!tokens) {
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  return std::move(callback).Run(std::move(tokens.value()), std::nullopt);
}

void PolkadotSubstrateRpc::GetFinalizedHead(std::string_view chain_id,
                                            GetFinalizedHeadCallback callback) {
  auto url = GetNetworkURL(chain_id);

  auto payload = base::WriteJson(
      MakeRpcRequestJson("chain_getFinalizedHead", base::ListValue()));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
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

  std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
  if (!PrefixedHexStringToFixed(*res->result, block_hash)) {
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
    params.Append(base::HexEncodeLower(*blockhash));
  }

  auto payload =
      base::WriteJson(MakeRpcRequestJson("chain_getHeader", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetBlockHeader,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetBlockHeader(GetBlockHeaderCallback callback,
                                            APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotChainHeader>(
          api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, not an error.
    return std::move(callback).Run(std::nullopt, std::nullopt);
  }

  auto header = ParseChainHeaderFromHex(res->result.value());
  if (!header) {
    // We received { "parentHash": "...", "number": "..." } that contained
    // invalid hex or hex that exceeded numeric limits.
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  std::move(callback).Run(std::move(*header), std::nullopt);
}

void PolkadotSubstrateRpc::GetBlock(
    std::string_view chain_id,
    std::optional<base::span<uint8_t, kPolkadotBlockHashSize>> block_hash,
    GetBlockCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;

  if (block_hash) {
    params.Append(base::HexEncodeLower(*block_hash));
  }

  auto payload =
      base::WriteJson(MakeRpcRequestJson("chain_getBlock", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetBlock,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetBlock(GetBlockCallback callback,
                                      APIRequestResult api_result) {
  auto res = HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotBlock>(
      api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, not an error.
    return std::move(callback).Run(std::nullopt, std::nullopt);
  }

  auto header = ParseChainHeaderFromHex(res->result->block.header);
  if (!header) {
    // We received { "parentHash": "...", "number": "..." } that contained
    // invalid hex or hex that exceeded numeric limits.
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  auto extrinsics = ParseExtrinsics(res->result->block);
  if (!extrinsics) {
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  PolkadotBlock block;
  block.header = std::move(header.value());
  block.extrinsics = std::move(extrinsics.value());

  std::move(callback).Run(std::move(block), std::nullopt);
}

void PolkadotSubstrateRpc::GetBlockHash(std::string_view chain_id,
                                        std::optional<uint32_t> block_number,
                                        GetBlockHashCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;

  if (block_number) {
    // The RPC nodes expect to see a block numbers like 13094409 as "00C7CE09".
    params.Append(base::HexEncode(base::U32ToBigEndian(*block_number)));
  }

  auto payload = base::WriteJson(
      MakeRpcRequestJson("chain_getBlockHash", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetBlockHash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetBlockHash(GetBlockHashCallback callback,
                                          APIRequestResult api_result) {
  auto res = HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotBlockHash>(
      api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, not an error.
    return std::move(callback).Run(std::nullopt, std::nullopt);
  }

  std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
  if (!PrefixedHexStringToFixed(*res->result, block_hash)) {
    // The JSON response contained invalid hex or too much/too little of it to
    // form a proper block hash from.
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  return std::move(callback).Run(block_hash, std::nullopt);
}

void PolkadotSubstrateRpc::GetRuntimeVersion(
    std::string_view chain_id,
    std::optional<base::span<uint8_t, kPolkadotBlockHashSize>> block_hash,
    GetRuntimeVersionCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;

  if (block_hash) {
    params.Append(base::HexEncodeLower(*block_hash));
  }

  auto payload = base::WriteJson(
      MakeRpcRequestJson("state_getRuntimeVersion", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetRuntimeVersion,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetRuntimeVersion(
    GetRuntimeVersionCallback callback,
    APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotRuntimeVersion>(
          api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, treat as an error for this
    // RPC call.
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  // Our IDL only permits us to work with signed integers, so we use checked
  // numerics to guarantee correct handling from signed -> unsigned.
  base::CheckedNumeric<uint32_t> spec_version = res->result->spec_version;
  base::CheckedNumeric<uint32_t> transaction_version =
      res->result->transaction_version;

  if (!spec_version.IsValid() || !transaction_version.IsValid()) {
    // The RPC documentation intends that these fields are U32 types, so if we
    // have invalid values here we can safely assume the parse is invalid.
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  PolkadotRuntimeVersion version;
  version.spec_version = spec_version.ValueOrDie();
  version.transaction_version = transaction_version.ValueOrDie();

  return std::move(callback).Run(version, std::nullopt);
}

void PolkadotSubstrateRpc::GetMetadata(std::string_view chain_id,
                                       GetMetadataCallback callback) {
  auto url = GetNetworkURL(chain_id);

  auto payload = base::WriteJson(
      MakeRpcRequestJson("state_getMetadata", base::ListValue()));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetMetadata(GetMetadataCallback callback,
                                         APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotMetadataResponse>(
          api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(base::unexpected(res.error()));
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, treat as an error for this
    // RPC call.
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  std::vector<uint8_t> metadata_bytes;
  if (!PrefixedHexStringToBytes(*res->result, &metadata_bytes)) {
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  return std::move(callback).Run(base::ok(std::move(metadata_bytes)));
}

void PolkadotSubstrateRpc::SubmitExtrinsic(std::string_view chain_id,
                                           std::string_view signed_extrinsic,
                                           SubmitExtrinsicCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;
  params.Append(signed_extrinsic);

  auto payload = base::WriteJson(
      MakeRpcRequestJson("author_submitExtrinsic", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnSubmitExtrinsic,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnSubmitExtrinsic(SubmitExtrinsicCallback callback,
                                             APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotSubmitExtrinsic>(
          api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(std::nullopt, res.error());
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, treat as an error for this
    // RPC call.
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  return std::move(callback).Run(*res->result, std::nullopt);
}

void PolkadotSubstrateRpc::GetPaymentInfo(std::string_view chain_id,
                                          base::span<const uint8_t> extrinsic,
                                          GetPaymentInfoCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;

  params.Append("TransactionPaymentApi_query_info");

  uint32_t size = 0;
  if (!base::CheckedNumeric<uint32_t>(extrinsic.size()).AssignIfValid(&size)) {
    return std::move(callback).Run(
        base::unexpected(WalletInternalErrorMessage()));
  }

  auto data = base::ToVector(extrinsic);
  base::Extend(data, base::byte_span_from_ref(size));

  params.Append(base::HexEncodeLower(data));

  auto payload =
      base::WriteJson(MakeRpcRequestJson("state_call", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetPaymentInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetPaymentInfo(GetPaymentInfoCallback callback,
                                            APIRequestResult api_result) {
  auto res =
      HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotPaymentInfo>(
          api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(base::unexpected(res.error()));
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, treat as an error for this
    // RPC call.
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  std::vector<uint8_t> query_info;
  if (!PrefixedHexStringToBytes(res->result.value(), &query_info)) {
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  std::array<uint8_t, 16> partial_fee_bytes = {};
  if (!parse_fee_info(::rust::Slice<const uint8_t>(query_info),
                      partial_fee_bytes)) {
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  return std::move(callback).Run(
      base::ok(base::bit_cast<uint128_t>(partial_fee_bytes)));
}

void PolkadotSubstrateRpc::GetEvents(
    std::string_view chain_id,
    base::span<const uint8_t, kPolkadotBlockHashSize> block_hash,
    GetEventsCallback callback) {
  auto url = GetNetworkURL(chain_id);

  base::ListValue params;

  // xxhash("System") | xxhash("Events")
  //
  // xxhashAsU8a(System, 128) => 0x26aa394eea5630e07c48ae0c9558cef7
  // xxhashAsU8a(Events, 128) => 0x80d41e5e16056765bc8461851072c9d7
  // https://github.com/polkadot-js/common/blob/047840319ef3f758880cc112b987888b8b2749d0/packages/util-crypto/src/xxhash/asU8a.ts#L24

  params.Append(
      "26aa394eea5630e07c48ae0c9558cef780d41e5e16056765bc8461851072c9d7");
  params.Append(base::HexEncodeLower(block_hash));

  auto payload = base::WriteJson(
      MakeRpcRequestJson("state_getStorage", std::move(params)));
  CHECK(payload);

  MakePostRequestInternal(
      url, *payload,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetEvents,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetEvents(GetEventsCallback callback,
                                       APIRequestResult api_result) {
  auto res = HandleRpcCall<polkadot_substrate_rpc_responses::PolkadotEvents>(
      api_result);

  if (!res.has_value()) {
    // We received either a network error, an actual RPC error or JSON that
    // didn't match our schema.
    return std::move(callback).Run(base::unexpected(res.error()));
  }

  if (!res->result) {
    // We received { "result": null } from the RPC, treat as an error for this
    // RPC call.
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  std::string_view events_str = res->result.value();
  if (events_str.starts_with("0x")) {
    events_str.remove_prefix(2);
  }

  std::vector<uint8_t> events;
  if (!base::HexStringToBytes(events_str, &events)) {
    // Returned string contained non-hex.
    return std::move(callback).Run(
        base::unexpected(WalletParsingErrorMessage()));
  }

  std::move(callback).Run(base::ok(std::move(events)));
}

GURL PolkadotSubstrateRpc::GetNetworkURL(std::string_view chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::DOT);
}

void PolkadotSubstrateRpc::MakePostRequestInternal(
    const GURL& url,
    const std::string& payload,
    api_request_helper::APIRequestHelper::ResultCallback callback) {
  api_request_helper_.Request(net::HttpRequestHeaders::kPostMethod, url,
                              payload, "application/json", std::move(callback),
                              MakePolkadotRpcHeaders(url));
}

}  // namespace brave_wallet
