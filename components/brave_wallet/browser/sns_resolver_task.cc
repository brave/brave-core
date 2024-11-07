/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/sns_resolver_task.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/numerics/byte_conversions.h"
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
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/solana_address.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "build/build_config.h"
#include "crypto/sha2.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {

struct SnsFetchRecordQueueItem {
  std::string record;
  SnsRecordsVersion version;
  SolanaAddress record_address;
};

namespace {

std::vector<SnsFetchRecordQueueItem> GetWalletAddressQueueRecords() {
  std::vector<SnsFetchRecordQueueItem> result;
  result.reserve(2);

  result.emplace_back();
  result.back().record = kSnsSolRecord;
  result.back().version = SnsRecordsVersion::kRecordsV2;

  result.emplace_back();
  result.back().record = kSnsSolRecord;
  result.back().version = SnsRecordsVersion::kRecordsV1;

  return result;
}

std::vector<SnsFetchRecordQueueItem> GetUrlQueueRecords() {
  std::vector<SnsFetchRecordQueueItem> result;
  result.reserve(4);

  result.emplace_back();
  result.back().record = kSnsUrlRecord;
  result.back().version = SnsRecordsVersion::kRecordsV2;

  result.emplace_back();
  result.back().record = kSnsIpfsRecord;
  result.back().version = SnsRecordsVersion::kRecordsV2;

  result.emplace_back();
  result.back().record = kSnsUrlRecord;
  result.back().version = SnsRecordsVersion::kRecordsV1;

  result.emplace_back();
  result.back().record = kSnsIpfsRecord;
  result.back().version = SnsRecordsVersion::kRecordsV1;

  return result;
}

SolanaAddress GetEmptyNameClass() {
  return SolanaAddress::ZeroAddress();
}

SolanaAddress GetCentalStateSnsRecordsNameClass() {
  // https://github.com/Bonfida/sns-sdk/blob/e930b83/rust-crates/sns-sdk/src/record/mod.rs#L10
  auto result =
      SolanaAddress::FromBase58("2pMnqHvei2N5oDcVGCRdZx48gqti199wr5CsyTTafsbo");
  CHECK(result);
  return *result;
}

template <class T>
std::optional<T> FromBase64(const std::string& str) {
  auto data = base::Base64Decode(str);
  if (!data) {
    return std::nullopt;
  }

  return T::FromBytes(*data);
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

struct SnsRecordV2 {
  SnsRecordV2ValidationType staleness_validation_type =
      SnsRecordV2ValidationType::kNone;
  SnsRecordV2ValidationType roa_validation_type =
      SnsRecordV2ValidationType::kNone;  // right_of_association
  uint32_t content_length = 0;
  SolanaAddress staleness_validation_id = SolanaAddress::ZeroAddress();
  SolanaAddress roa_validation_id = SolanaAddress::ZeroAddress();
  base::span<const uint8_t> content;
};

base::span<const uint8_t> ExtractSpan(base::span<const uint8_t>& data,
                                      size_t size) {
  if (data.size() < size) {
    return {};
  }
  auto result = data.subspan(0, size);
  data = data.subspan(size);
  return result;
}

bool FillValidationId(base::span<const uint8_t>& sol_record_payload,
                      SnsRecordV2ValidationType validation_type,
                      SolanaAddress& validation_id) {
  if (validation_type == SnsRecordV2ValidationType::kNone) {
    return true;
  } else if (validation_type == SnsRecordV2ValidationType::kSolana) {
    if (auto field = ExtractSpan(sol_record_payload, kSolanaPubkeySize);
        !field.empty()) {
      if (auto parsed_address = SolanaAddress::FromBytes(field)) {
        validation_id = std::move(*parsed_address);
        return true;
      }
    }
    return false;
  } else if (validation_type == SnsRecordV2ValidationType::kEthereum) {
    return !ExtractSpan(sol_record_payload, kEthAddressLength).empty();
  } else if (validation_type == SnsRecordV2ValidationType::kSolanaUnverified) {
    return !ExtractSpan(sol_record_payload, kSolanaPubkeySize).empty();
  }
  return false;
}

std::optional<SnsRecordV2> ParseSnsRecordV2(
    base::span<const uint8_t> sol_record_payload) {
  constexpr size_t kRecordV2HeaderSize =
      sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t);
  if (sol_record_payload.size() < kRecordV2HeaderSize) {
    return std::nullopt;
  }

  SnsRecordV2 result;

  if (auto field = ExtractSpan(sol_record_payload, sizeof(uint16_t));
      !field.empty()) {
    result.staleness_validation_type = static_cast<SnsRecordV2ValidationType>(
        base::U16FromNativeEndian(field.first<2u>()));
  } else {
    return std::nullopt;
  }

  if (auto field = ExtractSpan(sol_record_payload, sizeof(uint16_t));
      !field.empty()) {
    result.roa_validation_type = static_cast<SnsRecordV2ValidationType>(
        base::U16FromNativeEndian(field.first<2u>()));
  } else {
    return std::nullopt;
  }

  if (auto field = ExtractSpan(sol_record_payload, sizeof(uint32_t));
      !field.empty()) {
    result.content_length = base::U32FromNativeEndian(field.first<4u>());
  } else {
    return std::nullopt;
  }

  if (!FillValidationId(sol_record_payload, result.staleness_validation_type,
                        result.staleness_validation_id)) {
    return std::nullopt;
  }

  if (!FillValidationId(sol_record_payload, result.roa_validation_type,
                        result.roa_validation_id)) {
    return std::nullopt;
  }

  if (result.content_length != sol_record_payload.size()) {
    return std::nullopt;
  }

  result.content = sol_record_payload;

  return result;
}

// Parse SOL record data and verify signature.
// https://sns.guide/domain-name/records/records.html#the-sol-record-v1
std::optional<SolanaAddress> ParseAndVerifySolRecordV1Data(
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

std::optional<SolanaAddress> ParseAndVerifySolRecordV2Data(
    base::span<const uint8_t> sol_record_payload,
    const SolanaAddress& domain_owner) {
  auto sns_record_v2 = ParseSnsRecordV2(sol_record_payload);
  if (!sns_record_v2) {
    return std::nullopt;
  }

  auto result_address = SolanaAddress::FromBytes(sns_record_v2->content);
  if (!result_address) {
    return std::nullopt;
  }

  // https://github.com/Bonfida/sns-sdk/blob/0611a88/js/src/resolve.ts#L103-L108
  if (sns_record_v2->staleness_validation_type ==
          SnsRecordV2ValidationType::kSolana &&
      sns_record_v2->staleness_validation_id == domain_owner &&
      sns_record_v2->roa_validation_type ==
          SnsRecordV2ValidationType::kSolana &&
      sns_record_v2->roa_validation_id == *result_address) {
    return result_address;
  }

  return std::nullopt;
}

std::optional<SolanaAddress> ParseAndVerifySolRecordData(
    const SnsFetchRecordQueueItem& record_item,
    base::span<const uint8_t> sol_record_payload,
    const SolanaAddress& domain_owner) {
  if (record_item.version == SnsRecordsVersion::kRecordsV1) {
    return ParseAndVerifySolRecordV1Data(
        sol_record_payload, record_item.record_address, domain_owner);
  } else if (record_item.version == SnsRecordsVersion::kRecordsV2) {
    return ParseAndVerifySolRecordV2Data(sol_record_payload, domain_owner);
  }
  NOTREACHED();
}

std::optional<std::string> ParseAndVerifyTextRecordData(
    const SnsFetchRecordQueueItem& record_item,
    base::span<const uint8_t> sol_record_payload,
    const SolanaAddress& domain_owner) {
  if (record_item.version == SnsRecordsVersion::kRecordsV1) {
    // https://bonfida.github.io/solana-name-service-guide/registry.html
    // Parse NameRegistry data as a string trimming possible zeros at the end.
    return std::string(
        std::string(sol_record_payload.begin(), sol_record_payload.end())
            .c_str());
  } else if (record_item.version == SnsRecordsVersion::kRecordsV2) {
    auto sns_record_v2 = ParseSnsRecordV2(sol_record_payload);
    if (!sns_record_v2) {
      return std::nullopt;
    }

    // https://github.com/Bonfida/name-resolver/blob/b70089809/cf-worker-js/src/worker.ts#L86-L89
    if (sns_record_v2->staleness_validation_type ==
            SnsRecordV2ValidationType::kSolana &&
        sns_record_v2->staleness_validation_id == domain_owner) {
      return std::string(sns_record_v2->content.begin(),
                         sns_record_v2->content.end());
    }
    return std::nullopt;
  }
  NOTREACHED();
}

// https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L16
struct SplMintData {
  // Only interested in supply.
  uint64_t supply = 0;

  static std::optional<SplMintData> FromBytes(
      base::span<const uint8_t> data_span) {
    // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L37
    constexpr size_t kSplMintDataSize = 82;
    std::optional<SplMintData> result;

    if (data_span.size() != kSplMintDataSize) {
      return result;
    }

    result.emplace();
    // https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L41
    constexpr size_t kSupplyOffset = 36;
    result->supply =
        base::U64FromNativeEndian(data_span.subspan(kSupplyOffset).first<8u>());
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
    constexpr size_t kSplAccountDataSize = 165;
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
// Parsing result of getProgramAccounts call. Expected to find 1 token account
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
    const SolanaAddress& name_class,
    const std::optional<SolanaAddress>& parent) {
  if (!parent) {
    return std::nullopt;
  }

  std::vector<std::vector<uint8_t>> seeds;
  seeds.emplace_back(hashed_name.begin(), hashed_name.end());
  seeds.emplace_back(name_class.bytes());
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
SnsNamehash GetHashedName(const std::string& prefix, const std::string& name) {
  DCHECK_LE(prefix.size(), 1u);
  // https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/constants.ts#L13
  constexpr char kHashPrefix[] = "SPL Name Service";
  const std::string input = kHashPrefix + prefix + name;
  return crypto::SHA256Hash(base::as_byte_span(input));
}

// https://github.com/Bonfida/name-tokenizer#mint
std::optional<SolanaAddress> GetMintAddress(
    const SolanaAddress& domain_address) {
  constexpr char kMintPrefix[] = "tokenized_name";
  auto mint_prefix_bytes = base::span_from_cstring(kMintPrefix);

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

std::optional<SolanaAddress> GetDomainKey(const std::string& domain) {
  // https://github.com/Bonfida/solana-program-library/blob/6e3be3eedad3a7f4a83c1b7cd5f17f89231e0bca/name-service/js/src/constants.ts#L19
  if (domain == "sol") {
    return SolanaAddress::FromBase58(
        "58PwtjSDuFHuUkYjH9BYnnQKHfwo9reZhC2zMJv9JPkx");
  }

  const auto dot_count = base::ranges::count(domain, '.');
  if (dot_count > 2) {
    return std::nullopt;
  }

  auto dot_pos = domain.find('.');
  if (dot_pos == 0 || dot_pos == std::string::npos) {
    return std::nullopt;
  }

  // Subdomains get one zero byte prefix.
  // https://sns.guide/domain-name/records.html#difference-between-records-and-subdomains
  std::string parent = domain.substr(dot_pos + 1);
  std::string prefix = dot_count == 2 ? std::string(1, '\x00') : std::string();
  std::string name = domain.substr(0, dot_pos);

  return GetNameAccountKey(GetHashedName(prefix, name), GetEmptyNameClass(),
                           GetDomainKey(parent));
}

std::optional<SolanaAddress> GetRecordKey(const std::string& domain,
                                          const std::string& record,
                                          SnsRecordsVersion version) {
  // Records get one-byte prefix depending on requested record version.
  // https://sns.guide/domain-name/records.html#difference-between-records-and-subdomains
  std::string prefix = std::string(
      1, (version == SnsRecordsVersion::kRecordsV1 ? '\x01' : '\x02'));
  auto name_class = version == SnsRecordsVersion::kRecordsV1
                        ? GetEmptyNameClass()
                        : GetCentalStateSnsRecordsNameClass();

  return GetNameAccountKey(GetHashedName(prefix, record), name_class,
                           GetDomainKey(domain));
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
                                 TaskType type)
    : done_callback_(std::move(done_callback)),
      api_request_helper_(api_request_helper),
      domain_(domain),
      network_url_(network_url),
      task_type_(type) {
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

bool SnsResolverTask::FillWorkData() {
  auto domain_address = GetDomainKey(domain_);
  if (!domain_address) {
    return false;
  }
  domain_address_ = std::move(*domain_address);

  auto nft_mint_address = GetMintAddress(domain_address_);
  if (!nft_mint_address) {
    return false;
  }
  nft_mint_address_ = std::move(*nft_mint_address);

  if (task_type_ == TaskType::kResolveWalletAddress) {
    records_queue_ = GetWalletAddressQueueRecords();
  } else {
    records_queue_ = GetUrlQueueRecords();
  }
  cur_queue_item_pos_ = 0;

  for (auto& item : records_queue_) {
    if (auto address = GetRecordKey(domain_, item.record, item.version)) {
      item.record_address = std::move(*address);
    } else {
      return false;
    }
  }
  work_data_ready_ = true;
  return true;
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

  if (!work_data_ready_) {
    if (!FillWorkData()) {
      SetError(MakeInvalidParamsError());
      ScheduleWorkOnTask();
      return;
    }
  }

  if (!domain_name_registry_state_) {
    FetchDomainRegistryState();
    return;
  }

  // Check if there is an nft token associated with domain.
  if (!nft_owner_check_done_) {
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

  if (task_type_ == TaskType::kResolveWalletAddress) {
    WorkOnWalletAddressTask();
  } else {
    WorkOnDomainResolveTask();
  }
}

void SnsResolverTask::WorkOnWalletAddressTask() {
  // Use nft owner address as domain's SOL address.
  // https://github.com/Bonfida/sns-sdk/blob/0611a88/js/src/resolve.ts#L25-L27
  if (nft_owner_) {
    SetAddressResult(*nft_owner_);
    ScheduleWorkOnTask();
    return;
  }

  // No nft. Find owner of domain and contents of domain's SOL V2 or SOL V1
  // records.
  if (cur_queue_item_pos_ < records_queue_.size()) {
    FetchNextRecord();
    return;
  }

  SetAddressResult(domain_name_registry_state_->owner);
  ScheduleWorkOnTask();
}

void SnsResolverTask::WorkOnDomainResolveTask() {
  // Search for a valid url or ipfs records. Start with V2 records.
  if (cur_queue_item_pos_ < records_queue_.size()) {
    FetchNextRecord();
    return;
  }

  SetError(MakeInternalError());
  ScheduleWorkOnTask();
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
  nft_owner_ = std::move(nft_owner);
}

void SnsResolverTask::FetchNftSplMint() {
  DCHECK(!nft_owner_check_done_);
  DCHECK(!nft_mint_supply_check_done_);

  auto internal_callback = base::BindOnce(&SnsResolverTask::OnFetchNftSplMint,
                                          weak_ptr_factory_.GetWeakPtr());
  RequestInternal(solana::getAccountInfo(nft_mint_address_.ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchNftSplMint(APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

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
  auto internal_callback = base::BindOnce(
      &SnsResolverTask::OnFetchNftTokenOwner, weak_ptr_factory_.GetWeakPtr());
  RequestInternal(getProgramAccounts(nft_mint_address_),
                  std::move(internal_callback),
                  solana::ConverterForGetProgramAccounts());
}

void SnsResolverTask::OnFetchNftTokenOwner(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

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
  RequestInternal(solana::getAccountInfo(domain_address_.ToBase58()),
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchDomainRegistryState(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

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

void SnsResolverTask::FetchNextRecord() {
  CHECK_LT(cur_queue_item_pos_, records_queue_.size());

  auto internal_callback = base::BindOnce(&SnsResolverTask::OnFetchNextRecord,
                                          weak_ptr_factory_.GetWeakPtr());
  RequestInternal(
      solana::getAccountInfo(
          records_queue_[cur_queue_item_pos_].record_address.ToBase58()),
      std::move(internal_callback), solana::ConverterForGetAccountInfo());
}

void SnsResolverTask::OnFetchNextRecord(APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

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

  auto& cur_item = records_queue_[cur_queue_item_pos_];
  cur_queue_item_pos_++;

  if (!account_info) {
    // No such account for current record, go to the next record in queue.
    return;
  }

  auto record_name_registry_state =
      FromBase64<NameRegistryState>(account_info->data);
  if (!record_name_registry_state) {
    SetError(MakeInternalError());
    return;
  }

  if (cur_item.record == kSnsSolRecord) {
    DCHECK_EQ(task_type_, TaskType::kResolveWalletAddress);
    if (auto sol_record_payload_address = ParseAndVerifySolRecordData(
            cur_item, base::span(record_name_registry_state->data),
            domain_name_registry_state_->owner)) {
      SetAddressResult(*sol_record_payload_address);
      return;
    }
  } else if (cur_item.record == kSnsUrlRecord ||
             cur_item.record == kSnsIpfsRecord) {
    DCHECK_EQ(task_type_, TaskType::kResolveUrl);
    SolanaAddress domain_owner =
        nft_owner_ ? *nft_owner_ : domain_name_registry_state_->owner;
    auto registry_string = ParseAndVerifyTextRecordData(
        cur_item, base::span(record_name_registry_state->data), domain_owner);
    if (registry_string) {
      GURL ipfs_resolved_url;
      GURL url = (cur_item.record == kSnsIpfsRecord &&
                  ipfs::TranslateIPFSURI(GURL(*registry_string),
                                         &ipfs_resolved_url, false))
                     ? ipfs_resolved_url
                     : GURL(*registry_string);
      if (url.is_valid()) {
        SetUrlResult(std::move(url));
        return;
      }
    }
  } else {
    NOTREACHED_IN_MIGRATION();
  }
}

void SnsResolverTask::RequestInternal(
    const std::string& json_payload,
    RequestIntermediateCallback callback,
    ResponseConversionCallback conversion_callback) {
  api_request_helper_->Request(
      "POST", network_url_, json_payload, "application/json",
      std::move(callback), MakeCommonJsonRpcHeaders(json_payload, network_url_),
      {}, std::move(conversion_callback));
}

}  // namespace brave_wallet
