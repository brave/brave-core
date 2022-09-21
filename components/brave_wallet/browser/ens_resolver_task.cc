/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ens_resolver_task.h"

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {
namespace {

absl::optional<std::vector<uint8_t>> ExtractGatewayResult(
    const std::string& json) {
  auto records_v =
      base::JSONReader::Read(json, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    return absl::nullopt;
  }
  auto* data = records_v->GetDict().FindString("data");
  if (!data)
    return absl::nullopt;

  std::vector<uint8_t> result;
  if (!PrefixedHexStringToBytes(*data, &result))
    return absl::nullopt;
  return result;
}

EnsResolverTaskError ParseErrorResult(const std::string& json) {
  EnsResolverTaskError task_error;
  brave_wallet::ParseErrorResult<mojom::ProviderError>(
      json, &task_error.error, &task_error.error_message);

  return task_error;
}

EnsResolverTaskError MakeInternalError() {
  return EnsResolverTaskError(
      mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

}  // namespace

EnsResolverTaskResult::EnsResolverTaskResult() = default;
EnsResolverTaskResult::EnsResolverTaskResult(const EnsResolverTaskResult&) =
    default;
EnsResolverTaskResult::EnsResolverTaskResult(EnsResolverTaskResult&&) = default;
EnsResolverTaskResult& EnsResolverTaskResult::operator=(
    const EnsResolverTaskResult&) = default;
EnsResolverTaskResult& EnsResolverTaskResult::operator=(
    EnsResolverTaskResult&&) = default;
EnsResolverTaskResult::~EnsResolverTaskResult() = default;

EnsResolverTaskError::EnsResolverTaskError() = default;
EnsResolverTaskError::EnsResolverTaskError(mojom::ProviderError error,
                                           std::string error_message)
    : error(error), error_message(error_message) {}
EnsResolverTaskError::EnsResolverTaskError(const EnsResolverTaskError&) =
    default;
EnsResolverTaskError::EnsResolverTaskError(EnsResolverTaskError&&) = default;
EnsResolverTaskError& EnsResolverTaskError::operator=(
    const EnsResolverTaskError&) = default;
EnsResolverTaskError& EnsResolverTaskError::operator=(EnsResolverTaskError&&) =
    default;
EnsResolverTaskError::~EnsResolverTaskError() = default;

std::vector<uint8_t> MakeAddrCall(const std::string& domain) {
  return eth_abi::TupleEncoder()
      .AddFixedBytes(Namehash(domain))
      .EncodeWithSelector(base::make_span(kAddrBytes32Selector));
}

std::vector<uint8_t> MakeContentHashCall(const std::string& domain) {
  return eth_abi::TupleEncoder()
      .AddFixedBytes(Namehash(domain))
      .EncodeWithSelector(base::make_span(kContentHashBytes32Selector));
}

OffchainLookupData::OffchainLookupData() = default;
OffchainLookupData::OffchainLookupData(const OffchainLookupData&) = default;
OffchainLookupData::OffchainLookupData(OffchainLookupData&&) = default;
OffchainLookupData& OffchainLookupData::operator=(const OffchainLookupData&) =
    default;
OffchainLookupData& OffchainLookupData::operator=(OffchainLookupData&&) =
    default;
OffchainLookupData::~OffchainLookupData() = default;

absl::optional<OffchainLookupData> OffchainLookupData::ExtractFromJson(
    const std::string& json) {
  auto records_v =
      base::JSONReader::Read(json, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    return absl::nullopt;
  }

  auto* error_data = records_v->GetDict().FindStringByDottedPath("error.data");
  if (!error_data)
    return absl::nullopt;

  auto bytes = PrefixedHexStringToBytes(*error_data);
  if (!bytes) {
    return absl::nullopt;
  }

  return ExtractFromEthAbiPayload(*bytes);
}

absl::optional<OffchainLookupData> OffchainLookupData::ExtractFromEthAbiPayload(
    eth_abi::Span bytes) {
  auto [selector, args] =
      eth_abi::ExtractFunctionSelectorAndArgsFromCall(bytes);

  // error OffchainLookup(address sender, string[] urls, bytes callData,
  // bytes4 callbackFunction, bytes extraData)
  if (!base::ranges::equal(selector, kOffchainLookupSelector))
    return absl::nullopt;
  auto sender = eth_abi::ExtractAddressFromTuple(args, 0);
  auto urls = eth_abi::ExtractStringArrayFromTuple(args, 1);
  auto call_data = eth_abi::ExtractBytesFromTuple(args, 2);
  auto callback_function = eth_abi::ExtractFixedBytesFromTuple(args, 4, 3);
  auto extra_data = eth_abi::ExtractBytesFromTuple(args, 4);

  if (!sender.IsValid() || !urls || !call_data || !callback_function ||
      !extra_data) {
    return absl::nullopt;
  }

  OffchainLookupData result;
  result.sender = sender;
  result.urls = urls.value();
  result.call_data = call_data.value();
  result.callback_function = callback_function.value();
  result.extra_data = extra_data.value();
  return result;
}

class ScopedWorkOnTask {
 public:
  explicit ScopedWorkOnTask(EnsResolverTask* task) : task_(task) {}
  ~ScopedWorkOnTask() { task_->WorkOnTask(); }

 private:
  raw_ptr<EnsResolverTask> task_ = nullptr;
};

EnsResolverTask::EnsResolverTask(
    DoneCallback done_callback,
    APIRequestHelper* api_request_helper,
    APIRequestHelper* api_request_helper_ens_offchain,
    std::vector<uint8_t> ens_call,
    const std::string& domain,
    const GURL& network_url,
    absl::optional<bool> allow_offchain)
    : done_callback_(std::move(done_callback)),
      api_request_helper_(api_request_helper),
      api_request_helper_ens_offchain_(api_request_helper_ens_offchain),
      ens_call_(std::move(ens_call)),
      domain_(domain),
      network_url_(network_url),
      allow_offchain_(allow_offchain) {
  DCHECK(api_request_helper_);
  DCHECK(api_request_helper_ens_offchain_);
}

EnsResolverTask::~EnsResolverTask() = default;

void EnsResolverTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&EnsResolverTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::WorkOnTask() {
  if (task_result_) {
    std::move(done_callback_).Run(this, std::move(task_result_), absl::nullopt);
    // `this` is not valid here
    return;
  }
  if (task_error_) {
    std::move(done_callback_).Run(this, absl::nullopt, std::move(task_error_));
    // `this` is not valid here.
    return;
  }

  if (!resolver_address_.IsValid()) {
    FetchEnsResolver();
    return;
  }

  if (!supports_ensip_10_) {
    FetchEnsip10Support();
    return;
  }

  if (!supports_ensip_10_.value()) {
    FetchEnsRecord();
    return;
  }

  if (!offchain_lookup_data_) {
    FetchWithEnsip10Resolve();
    return;
  }

  if (offchain_lookup_attemps_left_ > 0) {
    FetchOffchainData();
    return;
  }

  std::move(done_callback_)
      .Run(this, absl::nullopt, EnsResolverTaskError(MakeInternalError()));
  // `this` is not valid here.
}

void EnsResolverTask::FetchEnsResolver() {
  DCHECK(resolver_address_.IsEmpty());
  const std::string contract_address =
      GetEnsRegistryContractAddress(brave_wallet::mojom::kMainnetChainId);

  std::string call_data = ens::Resolver(domain_);

  RequestInternal(eth::eth_call(contract_address, call_data),
                  base::BindOnce(&EnsResolverTask::OnFetchEnsResolverDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchEnsResolverDone(
    APIRequestResult api_request_result) {
  ScopedWorkOnTask work_on_task(this);

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.body());
    return;
  }

  auto resolver_address = eth_abi::ExtractAddressFromTuple(*bytes_result, 0);
  if (!resolver_address.IsValid() || resolver_address.IsZeroAddress()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  resolver_address_ = std::move(resolver_address);
}

void EnsResolverTask::FetchEnsip10Support() {
  DCHECK(resolver_address_.IsValid());

  // https://docs.ens.domains/ens-improvement-proposals/ensip-10-wildcard-resolution#specification
  auto call = erc165::SupportsInterface(kResolveBytesBytesSelector);

  RequestInternal(eth::eth_call(resolver_address_.ToHex(), ToHex(call)),
                  base::BindOnce(&EnsResolverTask::OnFetchEnsip10SupportDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchEnsip10SupportDone(
    APIRequestResult api_request_result) {
  ScopedWorkOnTask work_on_task(this);

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  bool is_supported = false;
  if (!ParseBoolResult(api_request_result.body(), &is_supported)) {
    task_error_ = ParseErrorResult(api_request_result.body());
    return;
  }

  supports_ensip_10_ = is_supported;
}

void EnsResolverTask::FetchEnsRecord() {
  DCHECK(resolver_address_.IsValid());
  DCHECK(supports_ensip_10_);
  DCHECK(!supports_ensip_10_.value());
  DCHECK(!task_result_);

  RequestInternal(eth::eth_call(resolver_address_.ToHex(), ToHex(ens_call_)),
                  base::BindOnce(&EnsResolverTask::OnFetchEnsRecordDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchEnsRecordDone(
    APIRequestResult api_request_result) {
  ScopedWorkOnTask work_on_task(this);

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.body());
    return;
  }

  task_result_.emplace();
  task_result_->resolved_result = std::move(*bytes_result);
}

void EnsResolverTask::FetchWithEnsip10Resolve() {
  DCHECK(resolver_address_.IsValid());
  DCHECK(supports_ensip_10_);
  DCHECK(supports_ensip_10_.value());
  DCHECK(!task_result_);

  if (!dns_encoded_name_) {
    dns_encoded_name_ = ens::DnsEncode(domain_);
    if (!dns_encoded_name_) {
      task_error_.emplace(MakeInternalError());
      ScheduleWorkOnTask();
      return;
    }
  }

  if (ens_resolve_call_.empty()) {
    ens_resolve_call_ = eth_abi::TupleEncoder()
                            .AddBytes(*dns_encoded_name_)
                            .AddBytes(ens_call_)
                            .EncodeWithSelector(kResolveBytesBytesSelector);
  }

  RequestInternal(
      eth::eth_call(resolver_address_.ToHex(), ToHex(ens_resolve_call_)),
      base::BindOnce(&EnsResolverTask::OnFetchWithEnsip10ResolveDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchWithEnsip10ResolveDone(
    APIRequestResult api_request_result) {
  ScopedWorkOnTask work_on_task(this);

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  if (auto offchain_lookup =
          OffchainLookupData::ExtractFromJson(api_request_result.body())) {
    offchain_lookup_data_ = std::move(offchain_lookup);
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.body());
    return;
  }

  // Decoding as returned bytes[] per
  // https://github.com/ensdomains/docs/blob/e4da40003943dd25fdf7d4c5552335330a9ee915/ens-improvement-proposals/ensip-10-wildcard-resolution.md?plain=1#L70
  auto decoded_resolve_result =
      eth_abi::ExtractBytesFromTuple(*bytes_result, 0);
  if (!decoded_resolve_result) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  task_result_.emplace();
  task_result_->resolved_result = std::move(*decoded_resolve_result);
}

void EnsResolverTask::FetchOffchainData() {
  DCHECK(offchain_lookup_data_);

  GURL offchain_url;
  bool data_substitued = false;
  bool valid_sender = true;

  if (!allow_offchain_.has_value()) {
    // No explicit offchain lookup decision. Will popup ui.
    task_result_.emplace();
    task_result_->need_to_allow_offchain = true;
    ScheduleWorkOnTask();
    return;
  } else if (!allow_offchain_.value()) {
    // Offchain lookup explicily disabled.
    task_error_.emplace(MakeInternalError());
    ScheduleWorkOnTask();
    return;
  }

  // Sender must match resolver per
  // https://eips.ethereum.org/EIPS/eip-3668#client-lookup-protocol #5.
  if (offchain_lookup_data_->sender != resolver_address_) {
    valid_sender = false;
  }

  // Pick first valid url.
  // TODO(apaymyshev): Implement picking different url per
  // https://eips.ethereum.org/EIPS/eip-3668#client-lookup-protocol #9.
  for (auto url_string : offchain_lookup_data_->urls) {
    base::ReplaceSubstringsAfterOffset(&url_string, 0, "{sender}",
                                       offchain_lookup_data_->sender.ToHex());
    data_substitued = base::Contains(url_string, "{data}");
    base::ReplaceSubstringsAfterOffset(&url_string, 0, "{data}",
                                       ToHex(offchain_lookup_data_->call_data));
    GURL url(url_string);
    if (url.is_valid() && url.SchemeIsHTTPOrHTTPS() &&
        url.SchemeIsCryptographic()) {
      offchain_url = std::move(url);
      break;
    }
  }

  if (!valid_sender || !offchain_url.is_valid()) {
    task_error_.emplace(MakeInternalError());
    ScheduleWorkOnTask();
    return;
  }

  std::string payload;
  if (!data_substitued) {
    base::Value::Dict payload_dict;
    payload_dict.Set("sender", offchain_lookup_data_->sender.ToHex());
    payload_dict.Set("data", ToHex(offchain_lookup_data_->call_data));
    base::JSONWriter::Write(payload_dict, &payload);
  }

  api_request_helper_ens_offchain_->Request(
      payload.empty() ? "GET" : "POST", offchain_url, payload,
      "application/json", false,
      base::BindOnce(&EnsResolverTask::OnFetchOffchainDone,
                     weak_ptr_factory_.GetWeakPtr()),
      {}, -1u, base::NullCallback());
}

void EnsResolverTask::OnFetchOffchainDone(APIRequestResult api_request_result) {
  ScopedWorkOnTask work_on_task(this);

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  auto bytes_result = ExtractGatewayResult(api_request_result.body());
  if (!bytes_result) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  offchain_lookup_attemps_left_--;
  DCHECK_GE(offchain_lookup_attemps_left_, 0);
  DCHECK_EQ(offchain_lookup_data_->callback_function.size(), 4u);
  eth_abi::Span4 callback_selector(
      offchain_lookup_data_->callback_function.begin(), 4);

  ens_resolve_call_ = eth_abi::TupleEncoder()
                          .AddBytes(*bytes_result)
                          .AddBytes(offchain_lookup_data_->extra_data)
                          .EncodeWithSelector(callback_selector);

  offchain_lookup_data_.reset();
}

void EnsResolverTask::RequestInternal(const std::string& json_payload,
                                      RequestIntermediateCallback callback) {
  api_request_helper_->Request("POST", network_url_, json_payload,
                               "application/json", false, std::move(callback),
                               MakeCommonJsonRpcHeaders(json_payload), -1u,
                               base::NullCallback());
}

}  // namespace brave_wallet
