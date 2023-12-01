/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/sns_resolver_task.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/sys_byteorder.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_requests.h"
#include "brave/components/brave_wallet/browser/solana_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "build/build_config.h"
#include "crypto/sha2.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {

namespace {

template <class T>
std::optional<T> FromBase64(const std::string& str) {
  auto data = base::Base64Decode(str);
  if (!data) {
    return std::nullopt;
  }

  return T::FromBytes(*data);
}

uint64_t FromLE(uint64_t uint64_le) {
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return uint64_le;
#else
  return base::ByteSwap(uint64_le);
#endif
}

SnsResolverTaskError ParseErrorResult(const base::Value& json_value) {
  SnsResolverTaskError task_error;
  brave_wallet::ParseErrorResult<mojom::SolanaProviderError>(
      json_value, &task_error.error, &task_error.error_message);

  return task_error;
}

SnsResolverTaskError MakeInternalError() {
  return SnsResolverTaskError(
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

SnsResolverTaskError MakeInvalidParamsError() {
  return SnsResolverTaskError(
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
}

// Parse SOL record data and verify signature.
// https://bonfida.github.io/solana-name-service-guide/domain-name/records.html#the-sol-record
std::optional<SolanaAddress> ParseAndVerifySolRecordData(
    base::span<const uint8_t> sol_record_payload,
    const SolanaAddress& sol_record_address,
    const SolanaAddress& domain_owner) {
  constexpr size_t kSolRecordDataSignature =
      static_cast<size_t>(ED25519_SIGNATURE_LEN);
  constexpr size_t kSolRecordDataSize =
      kSolanaPubkeySize + kSolRecordDataSignature;

  // No strict equality check here as `sol_record_payload` usually comes as 2K
  // bytes vector. We need only first 96 bytes of it.
  if (sol_record_payload.size() < kSolRecordDataSize) {
    return std::nullopt;
  }

  // Extract 32 bytes of address followed by 64 bytes of signature.
  auto sol_record_payload_address = SolanaAddress::FromBytes(
      sol_record_payload.subspan(0, kSolanaPubkeySize));
  if (!sol_record_payload_address) {
    return std::nullopt;
  }
  auto sol_record_payload_signature =
      sol_record_payload.subspan(kSolanaPubkeySize, kSolRecordDataSignature);

  std::vector<uint8_t> message;
  message.insert(message.end(), sol_record_payload_address->bytes().begin(),
                 sol_record_payload_address->bytes().end());
  message.insert(message.end(), sol_record_address.bytes().begin(),
                 sol_record_address.bytes().end());

  // Reference implementation adds lowered hex encoding.
  // https://github.com/Bonfida/solana-program-library/blob/171553544d76f5de294a0c041dfcb17834fe91c5/name-service/js/src/resolve.ts#L54
  std::string hex_message = base::ToLowerASCII(base::HexEncode(message));

  // Signature must match.
  if (!ED25519_verify(reinterpret_cast<const uint8_t*>(hex_message.data()),
                      hex_message.size(), sol_record_payload_signature.data(),
                      domain_owner.bytes().data())) {
    return std::nullopt;
  }

  return sol_record_payload_address;
}

// https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L16
struct SplMintData {
  // Only interested in supply.
  uint64_t supply = 0;

  static std::optional<SplMintData> FromBytes(
      base::span<const uint8_t> data_span) {
    // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L37
    const size_t kSplMintDataSize = 82;
    std::optional<SplMintData> result;

    if (data_span.size() != kSplMintDataSize) {
      return result;
    }

    result.emplace();
    // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L41
    const size_t supply_offset = 36;
    result->supply = FromLE(*reinterpret_cast<const uint64_t*>(
        data_span.subspan(supply_offset).data()));
    return result;
  }
};

// https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L86
struct SplAccountData {
  // Only interested in owner.
  SolanaAddress owner;

  static std::optional<SplAccountData> FromBytes(
      base::span<const uint8_t> data_span) {
    // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L129
    const size_t kSplAccountDataSize = 165;
    std::optional<SplAccountData> result;

    if (data_span.size() != kSplAccountDataSize) {
      return result;
    }

    result.emplace();
    // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L133
    const size_t owner_offset = 32;
    auto address = SolanaAddress::FromBytes(
        data_span.subspan(owner_offset, kSolanaPubkeySize));
    if (!address) {
      return std::nullopt;
    }
    result->owner = *address;
    return result;
  }
};

// Make getProgramAccounts RPC call to find token account for mint. Filters by
// token account data size, NFT amount eq to 1 and target mint address.
std::string getProgramAccounts(const SolanaAddress& mint_token) {
  base::Value::List params;
  params.Append(mojom::kSolanaTokenProgramId);

  base::Value::Dict configuration;
  configuration.Set("commitment", "confirmed");
  configuration.Set("encoding", "base64");

  // Offsets are within this struct:
  // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L86
  base::Value::List filters;
  // mint.
  filters.Append(base::Value::Dict());
  filters.back().GetDict().SetByDottedPath("memcmp.offset", 0);
  filters.back().GetDict().SetByDottedPath("memcmp.bytes",
                                           mint_token.ToBase58());
  // amount.
  filters.Append(base::Value::Dict());
  filters.back().GetDict().SetByDottedPath("memcmp.offset", 64);
  filters.back().GetDict().SetByDottedPath("memcmp.bytes",
                                           "2");  // base58 of 0x01
  filters.Append(base::Value::Dict());
  // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L129
  filters.back().GetDict().Set("dataSize", 165);
  configuration.Set("filters", std::move(filters));

  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("getProgramAccounts", std::move(params));
  return GetJSON(dictionary);
}

// https://docs.solana.com/developing/clients/jsonrpc-api#example-34
// Parsing result of getProgramAccounts call. Exepected to find 1 token account
// for mint. If parsing fails first element of pair is `false`.
std::pair<bool, std::optional<SolanaAddress>>
GetTokenOwnerFromGetProgramAccountsResult(const base::Value& json_value) {
  auto response = json_rpc_responses::RPCResponse::FromValue(json_value);
  if (!response || !response->result) {
    return {false, std::nullopt};
  }

  auto* result = response->result->GetIfList();
  if (!result) {
    return {false, std::nullopt};
  }

  if (result->size() != 1) {
    return {true, std::nullopt};
  }

  auto* item = result->front().GetIfDict();
  if (!item) {
    return {false, std::nullopt};
  }
  auto* account = item->FindDict("account");
  if (!account) {
    return {false, std::nullopt};
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfoPayload(*account, &account_info)) {
    return {false, std::nullopt};
  }

  DCHECK(account_info);

  auto account_data = FromBase64<SplAccountData>(account_info->data);
  if (!account_data) {
    return {false, std::nullopt};
  }

  return {true, account_data->owner};
}

// https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/utils.ts#L25
std::optional<SolanaAddress> GetNameAccountKey(
    const SnsNamehash& hashed_name,
    const std::optional<SolanaAddress>& parent) {
  if (!parent) {
    return std::nullopt;
  }

  std::vector<std::vector<uint8_t>> seeds;
  seeds.emplace_back(hashed_name.begin(), hashed_name.end());
  seeds.emplace_back(32, 0);  // we don't use nameClass here.
  seeds.emplace_back(parent->bytes());

  // https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/constants.ts#L7
  constexpr char kNameProgramId[] =
      "namesLPneVptA9Z5rqUDD9tMTWEJwofgaYwp8cawRkX";

  auto address =
      SolanaKeyring::FindProgramDerivedAddress(seeds, kNameProgramId);
  if (!address) {
    return std::nullopt;
  }

  return SolanaAddress::FromBase58(*address);
}

}  // namespace

// https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/utils.ts#L19
SnsNamehash GetHashedName(const std::string& name) {
  // https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/constants.ts#L13
  constexpr char kHashPrefix[] = "SPL Name Service";
  const std::string input = kHashPrefix + name;
  return crypto::SHA256Hash(base::as_bytes(base::make_span(input)));
}

// https://github.com/Bonfida/name-tokenizer#mint
std::optional<SolanaAddress> GetMintAddress(
    const SolanaAddress& domain_address) {
  const std::string kMintPrefix = "tokenized_name";
  auto mint_prefix_bytes = base::make_span(kMintPrefix);

  std::vector<std::vector<uint8_t>> seeds;
  seeds.emplace_back(mint_prefix_bytes.begin(), mint_prefix_bytes.end());
  seeds.emplace_back(domain_address.bytes());

  // https://github.com/Bonfida/name-tokenizer#program-id
  constexpr char kNameTokenizerId[] =
      "nftD3vbNkNqfj2Sd3HZwbpw4BxxKWr4AjGb9X38JeZk";

  auto address =
      SolanaKeyring::FindProgramDerivedAddress(seeds, kNameTokenizerId);
  if (!address) {
    return std::nullopt;
  }

  return SolanaAddress::FromBase58(*address);
}

// https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/utils.ts#L158
std::optional<SolanaAddress> GetDomainKey(const std::string& domain,
                                          bool record) {
  // https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/constants.ts#L19
  if (domain == "sol") {
    return SolanaAddress::FromBase58(
        "58PwtjSDuFHuUkYjH9BYnnQKHfwo9reZhC2zMJv9JPkx");
  }

  auto dot_pos = domain.find('.');
  if (dot_pos == std::string::npos) {
    DCHECK(false);
    return std::nullopt;
  }

  auto name = domain.substr(0, dot_pos);
  auto parent = domain.substr(dot_pos + 1);

  // Subdomains get one-bytes prefix depending on requested record type.
  // https://bonfida.github.io/solana-name-service-guide/domain-name/records.html#difference-between-records-and-subdomains
  std::string prefix = "";
  if (base::ranges::count(domain, '.') > 1) {
    prefix = (record ? '\x01' : '\x00');
  }

  return GetNameAccountKey(GetHashedName(prefix + name),
                           GetDomainKey(parent, false));
}

NameRegistryState::NameRegistryState() = default;
NameRegistryState::NameRegistryState(const NameRegistryState&) = default;
NameRegistryState::NameRegistryState(NameRegistryState&&) = default;
NameRegistryState& NameRegistryState::operator=(const NameRegistryState&) =
    default;
NameRegistryState& NameRegistryState::operator=(NameRegistryState&&) = default;
NameRegistryState::~NameRegistryState() = default;

// static
std::optional<NameRegistryState> NameRegistryState::FromBytes(
    base::span<const uint8_t> data_span) {
  // https://bonfida.github.io/solana-name-service-guide/registry.html
  const size_t kNameRegistryStateHeaderLength = 96;
  std::optional<NameRegistryState> result;
  if (data_span.size() < kNameRegistryStateHeaderLength) {
    return result;
  }

  // 96 bytes of header block followed by data block(possibly empty).
  result.emplace();
  result->parent_name =
      *SolanaAddress::FromBytes(data_span.subspan(0, kSolanaPubkeySize));
  data_span = data_span.subspan(kSolanaPubkeySize);

  result->owner =
      *SolanaAddress::FromBytes(data_span.subspan(0, kSolanaPubkeySize));
  data_span = data_span.subspan(kSolanaPubkeySize);

  result->data_class =
      *SolanaAddress::FromBytes(data_span.subspan(0, kSolanaPubkeySize));
  data_span = data_span.subspan(kSolanaPubkeySize);

  result->data.assign(data_span.begin(), data_span.end());
  return result;
}

SnsResolverTaskResult::SnsResolverTaskResult(SolanaAddress address)
    : resolved_address(std::move(address)) {}

SnsResolverTaskError::SnsResolverTaskError(mojom::SolanaProviderError error,
                                           std::string error_message)
    : error(error), error_message(error_message) {}

SnsResolverTask::SnsResolverTask(DoneCallback done_callback,
                                 APIRequestHelper* api_request_helper,
                                 const std::string& domain,
                                 const GURL& network_url,
                                 bool resolve_address)
    : done_callback_(std::move(done_callback)),
      api_request_helper_(api_request_helper),
      domain_(domain),
      network_url_(network_url),
      resolve_address_(resolve_address) {
  DCHECK(api_request_helper_);
}

SnsResolverTask::~SnsResolverTask() = default;

// static
base::RepeatingCallback<void(SnsResolverTask* task)>&
SnsResolverTask::GetWorkOnTaskForTesting() {
  static base::NoDestructor<
      base::RepeatingCallback<void(SnsResolverTask * task)>>
      callback;
  return *callback.get();
}

void SnsResolverTask::SetResultForTesting(
    std::optional<SnsResolverTaskResult> task_result,
    std::optional<SnsResolverTaskError> task_error) {
  task_result_ = std::move(task_result);
  task_error_ = std::move(task_error);
}

void SnsResolverTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&SnsResolverTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void SnsResolverTask::WorkOnTask() {
  if (!GetWorkOnTaskForTesting().is_null()) {
    GetWorkOnTaskForTesting().Run(this);
  }

  if (task_result_) {
    std::move(done_callback_).Run(this, std::move(task_result_), std::nullopt);
    // `this` is not valid here
    return;
  }
  if (task_error_) {
    std::move(done_callback_).Run(this, std::nullopt, std::move(task_error_));
    // `this` is not valid here.
    return;
  }

  if (!domain_address_) {
    domain_address_ = GetDomainKey(domain_, false);
    if (!domain_address_) {
      SetError(MakeInvalidParamsError());
      ScheduleWorkOnTask();
      return;
    }
  }

  if (resolve_address_) {
    WorkOnWalletAddressTask();
  } else {
    WorkOnDomainResolveTask();
  }
}

void SnsResolverTask::WorkOnWalletAddressTask() {
  // Check if there is an nft token associated with domain.
  if (!nft_owner_check_done_) {
    if (!nft_mint_address_) {
      nft_mint_address_ = GetMintAddress(*domain_address_);
      if (!nft_mint_address_) {
        NftOwnerDone(std::nullopt);
        ScheduleWorkOnTask();
        return;
      }
    }

    // Check if domain is minted and its supply is 1.
    if (!nft_mint_supply_check_done_) {
      FetchNftSplMint();
      return;
    }

    // Find token account for that mint and extract owner from its data to be
    // the resolution result.
    FetchNftTokenOwner();
    return;
  }

  // No nft. Find owner of domain and contents of domain's SOL record.
  if (!domain_name_registry_state_) {
    FetchDomainRegistryState();
    return;
  }

  FetchSolRecordRegistryState();
}

void SnsResolverTask::WorkOnDomainResolveTask() {
  if (!url_record_check_done_) {
    FetchUrlRecordRegistryState();
    return;
  }

  FetchIpfsRecordRegistryState();
}

void SnsResolverTask::SetAddressResult(SolanaAddress address) {
  task_result_.emplace();
  task_result_->resolved_address = std::move(address);
}

void SnsResolverTask::SetUrlResult(GURL url) {
  task_result_.emplace();
  task_result_->resolved_url = std::move(url);
}

void SnsResolverTask::SetError(SnsResolverTaskError error) {
  task_error_.emplace(std::move(error));
}

void SnsResolverTask::NftOwnerDone(std::optional<SolanaAddress> nft_owner) {
  nft_owner_check_done_ = true;
  if (nft_owner) {
    task_result_.emplace(*nft_owner);
  }
}

void SnsResolverTask::FetchNftSplMint() {
  DCHECK(domain_address_);
  DCHECK(!nft_owner_check_done_);
  DCHECK(!nft_mint_supply_check_done_);

  auto internal_callback = base::BindOnce(&SnsResolverTask::OnFetchNftSplMint,
                                          weak_ptr_factory_.GetWeakPtr());
  RequestInternal(solana::getAccountInfo(nft_mint_address_->ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchNftSplMint(APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this]() { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    SetError(MakeInternalError());
    return;
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.value_body(),
                                   &account_info)) {
    SetError(ParseErrorResult(api_request_result.value_body()));
    return;
  }

  if (!account_info) {
    NftOwnerDone(std::nullopt);
    return;
  }

  auto nft_mint = FromBase64<SplMintData>(account_info->data);
  if (!nft_mint || nft_mint->supply != 1) {
    NftOwnerDone(std::nullopt);
    return;
  }

  nft_mint_supply_check_done_ = true;
}

void SnsResolverTask::FetchNftTokenOwner() {
  DCHECK(nft_mint_address_);

  auto internal_callback = base::BindOnce(
      &SnsResolverTask::OnFetchNftTokenOwner, weak_ptr_factory_.GetWeakPtr());
  RequestInternal(getProgramAccounts(*nft_mint_address_),
                  std::move(internal_callback),
                  solana::ConverterForGetProrgamAccounts());
}

void SnsResolverTask::OnFetchNftTokenOwner(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this]() { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    SetError(MakeInternalError());
    return;
  }

  auto [parsing_ok, token_owner] = GetTokenOwnerFromGetProgramAccountsResult(
      api_request_result.value_body());
  if (!parsing_ok) {
    SetError(ParseErrorResult(api_request_result.value_body()));
    return;
  }

  NftOwnerDone(std::move(token_owner));
}

void SnsResolverTask::FetchDomainRegistryState() {
  DCHECK(!domain_name_registry_state_);

  auto internal_callback =
      base::BindOnce(&SnsResolverTask::OnFetchDomainRegistryState,
                     weak_ptr_factory_.GetWeakPtr());
  RequestInternal(solana::getAccountInfo(domain_address_->ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchDomainRegistryState(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this]() { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    SetError(MakeInternalError());
    return;
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.value_body(),
                                   &account_info)) {
    SetError(ParseErrorResult(api_request_result.value_body()));
    return;
  }

  if (!account_info) {
    SetError(MakeInternalError());
    return;
  }

  domain_name_registry_state_ =
      FromBase64<NameRegistryState>(account_info->data);
  if (!domain_name_registry_state_) {
    SetError(MakeInternalError());
    return;
  }
}

void SnsResolverTask::FetchSolRecordRegistryState() {
  auto sol_record_address = GetDomainKey("SOL." + domain_, true);
  if (!sol_record_address) {
    // Put the domain owner as a fallback result and use it if there is no SOL
    // record address could be extracted for any reason.
    task_result_.emplace(domain_name_registry_state_->owner);
    ScheduleWorkOnTask();
    return;
  }

  sol_record_address_ = std::move(*sol_record_address);

  auto internal_callback =
      base::BindOnce(&SnsResolverTask::OnFetchSolRecordRegistryState,
                     weak_ptr_factory_.GetWeakPtr());
  RequestInternal(solana::getAccountInfo(sol_record_address_.ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchSolRecordRegistryState(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this]() { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    SetError(MakeInternalError());
    return;
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.value_body(),
                                   &account_info)) {
    SetError(ParseErrorResult(api_request_result.value_body()));
    return;
  }

  if (!account_info) {
    // No such account, use owner address.
    SetAddressResult(domain_name_registry_state_->owner);
    return;
  }

  auto sol_record_name_registry_state =
      FromBase64<NameRegistryState>(account_info->data);
  if (!sol_record_name_registry_state) {
    SetError(MakeInternalError());
    return;
  }

  if (auto sol_record_payload_address = ParseAndVerifySolRecordData(
          base::make_span(sol_record_name_registry_state->data),
          sol_record_address_, domain_name_registry_state_->owner)) {
    SetAddressResult(*sol_record_payload_address);
  } else {
    // No valid address for SOL record, use owner address.
    SetAddressResult(domain_name_registry_state_->owner);
  }
}

void SnsResolverTask::FetchUrlRecordRegistryState() {
  auto url_record_address = GetDomainKey("url." + domain_, true);
  if (!url_record_address) {
    SetError(MakeInternalError());
    ScheduleWorkOnTask();
    return;
  }

  auto internal_callback =
      base::BindOnce(&SnsResolverTask::OnFetchUrlRecordRegistryState,
                     weak_ptr_factory_.GetWeakPtr());
  RequestInternal(solana::getAccountInfo(url_record_address->ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchUrlRecordRegistryState(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this]() { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    SetError(MakeInternalError());
    return;
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.value_body(),
                                   &account_info)) {
    SetError(ParseErrorResult(api_request_result.value_body()));
    return;
  }

  if (!account_info) {
    // No url record account, will proceed with ipfs record.
    url_record_check_done_ = true;
    return;
  }

  auto url_record_name_registry_state =
      FromBase64<NameRegistryState>(account_info->data);
  if (!url_record_name_registry_state) {
    SetError(MakeInternalError());
    return;
  }

  // https://bonfida.github.io/solana-name-service-guide/registry.html
  // Parse NameRegistry data as a string trimming possible zeros at the end.
  std::string url_string(
      std::string(url_record_name_registry_state->data.begin(),
                  url_record_name_registry_state->data.end())
          .c_str());

  GURL url(url_string);
  if (!url.is_valid()) {
    SetError(MakeInternalError());
    return;
  }

  SetUrlResult(std::move(url));
}

void SnsResolverTask::FetchIpfsRecordRegistryState() {
  auto ipfs_record_address = GetDomainKey("IPFS." + domain_, true);
  if (!ipfs_record_address) {
    SetError(MakeInternalError());
    ScheduleWorkOnTask();
    return;
  }

  auto internal_callback =
      base::BindOnce(&SnsResolverTask::OnFetchIpfsRecordRegistryState,
                     weak_ptr_factory_.GetWeakPtr());
  RequestInternal(solana::getAccountInfo(ipfs_record_address->ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchIpfsRecordRegistryState(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this]() { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    SetError(MakeInternalError());
    return;
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.value_body(),
                                   &account_info)) {
    SetError(ParseErrorResult(api_request_result.value_body()));
    return;
  }

  if (!account_info) {
    // No ipfs record account, resolve as error.
    SetError(MakeInternalError());
    return;
  }

  auto ipfs_record_name_registry_state =
      FromBase64<NameRegistryState>(account_info->data);
  if (!ipfs_record_name_registry_state) {
    SetError(MakeInternalError());
    return;
  }

  // https://bonfida.github.io/solana-name-service-guide/registry.html
  // Parse NameRegistry data as a string trimming possible zeros at the end.
  std::string url_string(
      std::string(ipfs_record_name_registry_state->data.begin(),
                  ipfs_record_name_registry_state->data.end())
          .c_str());

  GURL url(url_string);
  if (!url.is_valid()) {
    SetError(MakeInternalError());
    return;
  }

  SetUrlResult(std::move(url));
}

void SnsResolverTask::RequestInternal(
    const std::string& json_payload,
    RequestIntermediateCallback callback,
    ResponseConversionCallback conversion_callback) {
  api_request_helper_->Request("POST", network_url_, json_payload,
                               "application/json", std::move(callback),
                               MakeCommonJsonRpcHeaders(json_payload), {},
                               std::move(conversion_callback));
}

}  // namespace brave_wallet
