/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ens_resolver_task.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/compiler_specific.h"
#include "base/containers/contains.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/values.h"
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
#include "components/grit/brave_components_strings.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"

namespace brave_wallet {
namespace {

std::optional<std::vector<uint8_t>> ExtractGatewayResult(
    const base::Value& json_value) {
  if (!json_value.is_dict()) {
    return std::nullopt;
  }
  auto* data = json_value.GetDict().FindString("data");
  if (!data) {
    return std::nullopt;
  }

  std::vector<uint8_t> result;
  if (!PrefixedHexStringToBytes(*data, &result)) {
    return std::nullopt;
  }
  return result;
}

EnsResolverTaskError ParseErrorResult(const base::Value& json_value) {
  EnsResolverTaskError task_error;
  brave_wallet::ParseErrorResult<mojom::ProviderError>(
      json_value, &task_error.error, &task_error.error_message);

  return task_error;
}

EnsResolverTaskError MakeInternalError() {
  return EnsResolverTaskError(
      mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

EnsResolverTaskError MakeInvalidParamsError() {
  return EnsResolverTaskError(
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
}

std::string GetParent(const std::string& domain) {
  DCHECK(domain == "eth" || domain.ends_with(".eth"));
  if (domain == "eth") {
    return "";
  }
  std::size_t dot_pos = domain.find('.');
  if (dot_pos == std::string::npos) {
    return "";
  }
  return domain.substr(dot_pos + 1);
}

}  // namespace

EnsResolverTaskResult::EnsResolverTaskResult() = default;
EnsResolverTaskResult::EnsResolverTaskResult(
    std::vector<uint8_t> resolved_result,
    bool need_to_allow_offchain)
    : resolved_result(std::move(resolved_result)),
      need_to_allow_offchain(need_to_allow_offchain) {}
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
      .EncodeWithSelector(kAddrBytes32Selector);
}

std::vector<uint8_t> MakeContentHashCall(const std::string& domain) {
  return eth_abi::TupleEncoder()
      .AddFixedBytes(Namehash(domain))
      .EncodeWithSelector(kContentHashBytes32Selector);
}

OffchainLookupData::OffchainLookupData() = default;
OffchainLookupData::OffchainLookupData(const OffchainLookupData&) = default;
OffchainLookupData::OffchainLookupData(OffchainLookupData&&) = default;
OffchainLookupData& OffchainLookupData::operator=(const OffchainLookupData&) =
    default;
OffchainLookupData& OffchainLookupData::operator=(OffchainLookupData&&) =
    default;
OffchainLookupData::~OffchainLookupData() = default;

std::optional<OffchainLookupData> OffchainLookupData::ExtractFromJson(
    const base::Value& json_value) {
  if (!json_value.is_dict()) {
    return std::nullopt;
  }

  auto* error_data = json_value.GetDict().FindStringByDottedPath("error.data");
  if (!error_data) {
    return std::nullopt;
  }

  auto bytes = PrefixedHexStringToBytes(*error_data);
  if (!bytes) {
    return std::nullopt;
  }

  return ExtractFromEthAbiPayload(*bytes);
}

std::optional<OffchainLookupData> OffchainLookupData::ExtractFromEthAbiPayload(
    eth_abi::Span bytes) {
  auto selector_and_args =
      eth_abi::ExtractFunctionSelectorAndArgsFromCall(bytes);
  if (!selector_and_args) {
    return std::nullopt;
  }
  auto [selector, args] = *selector_and_args;

  // error OffchainLookup(address sender, string[] urls, bytes callData,
  // bytes4 callbackFunction, bytes extraData)
  if (selector != kOffchainLookupSelector) {
    return std::nullopt;
  }
  auto sender = eth_abi::ExtractAddressFromTuple(args, 0);
  auto urls = eth_abi::ExtractStringArrayFromTuple(args, 1);
  auto call_data = eth_abi::ExtractBytesFromTuple(args, 2);
  auto callback_function = eth_abi::ExtractFixedBytesFromTuple<4>(args, 3);
  auto extra_data = eth_abi::ExtractBytesFromTuple(args, 4);

  if (!sender.IsValid() || !urls || !call_data || !callback_function ||
      !extra_data) {
    return std::nullopt;
  }

  OffchainLookupData result;
  result.sender = sender;
  result.urls = urls.value();
  result.call_data = call_data.value();
  result.callback_function = callback_function.value();
  result.extra_data = extra_data.value();
  return result;
}

EnsResolverTask::EnsResolverTask(
    DoneCallback done_callback,
    APIRequestHelper* api_request_helper,
    APIRequestHelper* api_request_helper_ens_offchain,
    std::vector<uint8_t> ens_call,
    const std::string& domain,
    const GURL& network_url,
    std::optional<bool> allow_offchain)
    : done_callback_(std::move(done_callback)),
      api_request_helper_(api_request_helper),
      api_request_helper_ens_offchain_(api_request_helper_ens_offchain),
      ens_call_(std::move(ens_call)),
      domain_(domain),
      resolver_domain_(domain),
      network_url_(network_url),
      allow_offchain_(allow_offchain) {
  DCHECK(api_request_helper_);
  DCHECK(api_request_helper_ens_offchain_);
}

EnsResolverTask::~EnsResolverTask() = default;

// static
base::RepeatingCallback<void(EnsResolverTask* task)>&
EnsResolverTask::GetWorkOnTaskForTesting() {
  static base::NoDestructor<
      base::RepeatingCallback<void(EnsResolverTask * task)>>
      callback;
  return *callback.get();
}

void EnsResolverTask::SetResultForTesting(
    std::optional<EnsResolverTaskResult> task_result,
    std::optional<EnsResolverTaskError> task_error) {
  task_result_ = std::move(task_result);
  task_error_ = std::move(task_error);
}

void EnsResolverTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&EnsResolverTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::WorkOnTask() {
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

  if (offchain_lookup_data_) {
    FetchOffchainData();
    return;
  }

  if (offchain_callback_call_) {
    FetchOffchainCallback();
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

  // Both of these calls could result with either
  // - `task_result_` filled with abi-encoded requested record and done, or
  // - `task_result_` filled with `need_to_allow_offchain` flag set and done, or
  // - `task_error_` filled with error and done, or
  // - non-empty `offchain_lookup_data_` to start offchain lookup iterations.
  if (!supports_ensip_10_.value()) {
    FetchEnsRecord();
  } else {
    FetchWithEnsip10Resolve();
  }
}

void EnsResolverTask::FetchEnsResolver() {
  DCHECK(resolver_address_.IsEmpty());
  const std::string contract_address =
      GetEnsRegistryContractAddress(brave_wallet::mojom::kMainnetChainId);

  std::string call_data = ens::Resolver(resolver_domain_);

  RequestInternal(eth::eth_call(contract_address, call_data),
                  base::BindOnce(&EnsResolverTask::OnFetchEnsResolverDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchEnsResolverDone(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.value_body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.value_body());
    return;
  }

  auto resolver_address = eth_abi::ExtractAddressFromTuple(*bytes_result, 0);
  if (!resolver_address.IsValid()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  if (resolver_address.IsZeroAddress()) {
    auto parent = GetParent(resolver_domain_);
    if (parent.empty()) {
      task_error_.emplace(MakeInternalError());
      return;
    } else {
      resolver_domain_ = parent;
      return;
    }
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
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  auto is_supported = ParseBoolResult(api_request_result.value_body());
  if (!is_supported.has_value()) {
    task_error_ = ParseErrorResult(api_request_result.value_body());
    return;
  }

  supports_ensip_10_ = is_supported.value();
}

void EnsResolverTask::FetchEnsRecord() {
  DCHECK(resolver_address_.IsValid());
  DCHECK(supports_ensip_10_);
  DCHECK(!supports_ensip_10_.value());
  DCHECK(!task_result_);

  if (domain_ != resolver_domain_) {
    // Wildcard resolution is supported only with resolve(bytes,bytes) method.
    task_error_.emplace(MakeInvalidParamsError());
    ScheduleWorkOnTask();
    return;
  }

  RequestInternal(eth::eth_call(resolver_address_.ToHex(), ToHex(ens_call_)),
                  base::BindOnce(&EnsResolverTask::OnFetchEnsRecordDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchEnsRecordDone(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  if (auto offchain_lookup = OffchainLookupData::ExtractFromJson(
          api_request_result.value_body())) {
    offchain_lookup_data_ = std::move(offchain_lookup);
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.value_body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.value_body());
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

  auto ens_resolve_call = eth_abi::TupleEncoder()
                              .AddBytes(*dns_encoded_name_)
                              .AddBytes(ens_call_)
                              .EncodeWithSelector(kResolveBytesBytesSelector);

  RequestInternal(
      eth::eth_call(resolver_address_.ToHex(), ToHex(ens_resolve_call)),
      base::BindOnce(&EnsResolverTask::OnFetchWithEnsip10ResolveDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchWithEnsip10ResolveDone(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  if (auto offchain_lookup = OffchainLookupData::ExtractFromJson(
          api_request_result.value_body())) {
    offchain_lookup_data_ = std::move(offchain_lookup);
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.value_body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.value_body());
    return;
  }

  DCHECK(supports_ensip_10_.value());
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
  bool data_substituted = false;
  bool valid_sender = true;

  if (!allow_offchain_.has_value()) {
    // No explicit offchain lookup decision. Will popup ui.
    task_result_.emplace();
    task_result_->need_to_allow_offchain = true;
    ScheduleWorkOnTask();
    return;
  } else if (!allow_offchain_.value()) {
    // Offchain lookup explicitly disabled.
    task_error_.emplace(MakeInternalError());
    ScheduleWorkOnTask();
    return;
  }

  if (offchain_lookup_attemps_left_ <= 0) {
    // No more attempts left results in a error.
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
    data_substituted = base::Contains(url_string, "{data}");
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
  if (!data_substituted) {
    base::Value::Dict payload_dict;
    payload_dict.Set("sender", offchain_lookup_data_->sender.ToHex());
    payload_dict.Set("data", ToHex(offchain_lookup_data_->call_data));
    base::JSONWriter::Write(payload_dict, &payload);
  }

  api_request_helper_ens_offchain_->Request(
      payload.empty() ? "GET" : "POST", offchain_url, payload,
      "application/json",
      base::BindOnce(&EnsResolverTask::OnFetchOffchainDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchOffchainDone(APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  auto bytes_result = ExtractGatewayResult(api_request_result.value_body());
  if (!bytes_result) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  offchain_lookup_attemps_left_--;
  DCHECK_GE(offchain_lookup_attemps_left_, 0);
  DCHECK_EQ(offchain_lookup_data_->callback_function.size(), 4u);
  UNSAFE_TODO(eth_abi::Span4 callback_selector(
      offchain_lookup_data_->callback_function.begin(), 4u));

  offchain_callback_call_ = eth_abi::TupleEncoder()
                                .AddBytes(*bytes_result)
                                .AddBytes(offchain_lookup_data_->extra_data)
                                .EncodeWithSelector(callback_selector);

  offchain_lookup_data_.reset();
}

void EnsResolverTask::FetchOffchainCallback() {
  DCHECK(resolver_address_.IsValid());
  DCHECK(!offchain_lookup_data_);
  DCHECK(offchain_callback_call_);
  DCHECK(!task_result_);

  RequestInternal(
      eth::eth_call(resolver_address_.ToHex(), ToHex(*offchain_callback_call_)),
      base::BindOnce(&EnsResolverTask::OnFetchOffchainCallbackDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EnsResolverTask::OnFetchOffchainCallbackDone(
    APIRequestResult api_request_result) {
  absl::Cleanup cleanup([this] { this->WorkOnTask(); });

  if (!api_request_result.Is2XXResponseCode()) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  offchain_callback_call_.reset();

  if (auto offchain_lookup = OffchainLookupData::ExtractFromJson(
          api_request_result.value_body())) {
    offchain_lookup_data_ = std::move(offchain_lookup);
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(api_request_result.value_body());
  if (!bytes_result) {
    task_error_ = ParseErrorResult(api_request_result.value_body());
    return;
  }

  std::optional<std::vector<uint8_t>> decoded_resolve_result;
  if (supports_ensip_10_.value()) {
    // Decoding as returned bytes[] per
    // https://github.com/ensdomains/docs/blob/e4da40003943dd25fdf7d4c5552335330a9ee915/ens-improvement-proposals/ensip-10-wildcard-resolution.md?plain=1#L70
    decoded_resolve_result = eth_abi::ExtractBytesFromTuple(*bytes_result, 0);
  } else {
    decoded_resolve_result = *bytes_result;
  }
  if (!decoded_resolve_result) {
    task_error_.emplace(MakeInternalError());
    return;
  }

  task_result_.emplace();
  task_result_->resolved_result = std::move(*decoded_resolve_result);
}

void EnsResolverTask::RequestInternal(const std::string& json_payload,
                                      RequestIntermediateCallback callback) {
  api_request_helper_->Request(
      "POST", network_url_, json_payload, "application/json",
      std::move(callback),
      MakeCommonJsonRpcHeaders(json_payload, network_url_));
}

}  // namespace brave_wallet
