/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ens_resolver_task.h"

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "base/containers/contains.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave_wallet {
namespace {
// resolve(bytes32,bytes32)
const uint8_t kResolveBytes32Bytes32Hash[] = {0x90, 0x61, 0xb9, 0x23};

absl::optional<OffchainLookupData> ExtractOffchainLookup(
    const std::string& json) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v || !records_v->is_dict()) {
    return absl::nullopt;
  }

  auto* error_data = records_v->FindStringPath("error.data");
  if (!error_data)
    return absl::nullopt;

  auto bytes = PrefixedHexStringToBytes(*error_data);
  if (!bytes) {
    return absl::nullopt;
  }

  auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(*bytes);

  // error OffchainLookup(address sender, string[] urls, bytes callData,
  // bytes4 callbackFunction, bytes extraData)
  if (ToHex(selector) != "0x556f1830")
    return absl::nullopt;
  auto sender = ExtractAddressFromTuple(args, 0);
  auto urls = ExtractStringArrayFromTuple(args, 1);
  auto call_data = ExtractBytesFromTuple(args, 2);
  auto callback_function = ExtractFixedBytesFromTuple(args, 4, 3);
  auto extra_data = ExtractBytesFromTuple(args, 4);

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

absl::optional<std::vector<uint8_t>> ExtractGatewayResult(
    const std::string& json) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v || !records_v->is_dict()) {
    return absl::nullopt;
  }
  auto* data = records_v->FindStringPath("data");
  if (!data)
    return absl::nullopt;

  std::vector<uint8_t> result;
  if (!PrefixedHexStringToBytes(*data, &result))
    return absl::nullopt;
  return result;
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ens_resolver", R"(
      semantics {
        sender: "ENS Resolver Task"
        description:
          "Fetches ENS offchain data."
        trigger:
          "Triggerer by ENS offchain lookup."
        data:
          "Offchain lookup info."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable ENS on brave://settings/extensions page."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

std::unique_ptr<network::SimpleURLLoader> CreateLoader(
    const GURL& url,
    const std::string& payload) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = payload.empty() ? "GET" : "POST";

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!payload.empty()) {
    url_loader->AttachStringForUpload(payload, "application/json");
  }
  return url_loader;
}

}  // namespace

OffchainLookupData::OffchainLookupData() = default;
OffchainLookupData::OffchainLookupData(const OffchainLookupData&) = default;
OffchainLookupData::OffchainLookupData(OffchainLookupData&&) = default;
OffchainLookupData& OffchainLookupData::operator=(const OffchainLookupData&) =
    default;
OffchainLookupData& OffchainLookupData::operator=(OffchainLookupData&&) =
    default;
OffchainLookupData::~OffchainLookupData() = default;

class ScopedWorkOnTask {
 public:
  explicit ScopedWorkOnTask(EnsGetEthAddrTask* task) : task_(task) {}
  ~ScopedWorkOnTask() { task_->WorkOnTask(); }

 private:
  raw_ptr<EnsGetEthAddrTask> task_ = nullptr;
};

EnsGetEthAddrTask::EnsGetEthAddrTask(
    JsonRpcServiceBase* json_rpc_service_base,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::vector<uint8_t> ens_call,
    const std::string& domain,
    const std::vector<uint8_t>& dns_encoded_name,
    const GURL& network_url)
    : json_rpc_service_base_(json_rpc_service_base),
      url_loader_factory_(url_loader_factory),
      ens_call_(std::move(ens_call)),
      domain_(domain),
      dns_encoded_name_(dns_encoded_name),
      network_url_(network_url) {}

EnsGetEthAddrTask::~EnsGetEthAddrTask() = default;

void EnsGetEthAddrTask::WorkOnTask() {
  if (resolve_result_) {
    json_rpc_service_base_->OnEnsResolverTaskDone(
        this, std::move(resolve_result_.value()),
        mojom::ProviderError::kSuccess, "");
    // `this` is not valid here
    return;
  }
  if (error_ || error_message_) {
    json_rpc_service_base_->OnEnsResolverTaskDone(
        this, {}, error_.value_or(mojom::ProviderError::kInternalError),
        error_message_.value_or(""));
    // `this` is not valid here.
    return;
  }

  if (!resolver_) {
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

  json_rpc_service_base_->OnEnsResolverTaskDone(
      this, {}, mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  // `this` is not valid here.
}

void EnsGetEthAddrTask::FetchEnsResolver() {
  DCHECK(!resolver_);
  const std::string contract_address =
      GetEnsRegistryContractAddress(brave_wallet::mojom::kMainnetChainId);

  std::string call_data = ens::Resolver(domain_);

  auto internal_callback =
      base::BindOnce(&EnsGetEthAddrTask::OnFetchEnsResolverDone,
                     weak_ptr_factory_.GetWeakPtr());
  json_rpc_service_base_->RequestInternal(
      eth::eth_call(contract_address, call_data), true, network_url_,
      std::move(internal_callback), base::NullCallback());
}

void EnsGetEthAddrTask::OnFetchEnsResolverDone(
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  ScopedWorkOnTask work_on_task(this);

  if (status < 200 || status > 299) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  std::string resolver_address;
  if (!eth::ParseAddressResult(body, &resolver_address) ||
      resolver_address.empty()) {
    ParseErrorResult<mojom::ProviderError>(body, &error_.emplace(),
                                           &error_message_.emplace());
    return;
  }

  resolver_ = resolver_address;
}

void EnsGetEthAddrTask::FetchEnsip10Support() {
  DCHECK(resolver_);

  // https://docs.ens.domains/ens-improvement-proposals/ensip-10-wildcard-resolution#specification
  std::string data = erc165::SupportsInterface("0x9061b923");

  auto internal_callback =
      base::BindOnce(&EnsGetEthAddrTask::OnFetchEnsip10SupportDone,
                     weak_ptr_factory_.GetWeakPtr());
  json_rpc_service_base_->RequestInternal(
      eth::eth_call(*resolver_, data), true, network_url_,
      std::move(internal_callback), base::NullCallback());
}

void EnsGetEthAddrTask::OnFetchEnsip10SupportDone(
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  ScopedWorkOnTask work_on_task(this);

  if (status < 200 || status > 299) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  bool is_supported = false;
  if (!ParseBoolResult(body, &is_supported)) {
    ParseErrorResult<mojom::ProviderError>(body, &error_.emplace(),
                                           &error_message_.emplace());
    return;
  }

  supports_ensip_10_ = is_supported;
}

void EnsGetEthAddrTask::FetchEnsRecord() {
  DCHECK(resolver_);
  DCHECK(supports_ensip_10_);
  DCHECK(!supports_ensip_10_.value());
  DCHECK(!resolve_result_);

  auto internal_callback = base::BindOnce(
      &EnsGetEthAddrTask::OnFetchEnsRecordDone, weak_ptr_factory_.GetWeakPtr());
  json_rpc_service_base_->RequestInternal(
      eth::eth_call(*resolver_, ToHex(ens_call_)), true, network_url_,
      std::move(internal_callback), base::NullCallback());
}

void EnsGetEthAddrTask::OnFetchEnsRecordDone(
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  ScopedWorkOnTask work_on_task(this);

  if (status < 200 || status > 299) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(body);
  if (!bytes_result) {
    ParseErrorResult<mojom::ProviderError>(body, &error_.emplace(),
                                           &error_message_.emplace());
    return;
  }

  resolve_result_ = std::move(bytes_result);
}

void EnsGetEthAddrTask::FetchWithEnsip10Resolve() {
  DCHECK(resolver_);
  DCHECK(supports_ensip_10_);
  DCHECK(supports_ensip_10_.value());
  DCHECK(!resolve_result_);

  if (ens_resolve_call_.empty()) {
    ens_resolve_call_ = EncodeCall(base::make_span(kResolveBytes32Bytes32Hash),
                                   base::make_span(dns_encoded_name_),
                                   base::make_span(ens_call_));
  }

  auto internal_callback =
      base::BindOnce(&EnsGetEthAddrTask::OnFetchWithEnsip10ResolveDone,
                     weak_ptr_factory_.GetWeakPtr());
  json_rpc_service_base_->RequestInternal(
      eth::eth_call(*resolver_, ToHex(ens_resolve_call_)), true, network_url_,
      std::move(internal_callback), base::NullCallback());
}

void EnsGetEthAddrTask::OnFetchWithEnsip10ResolveDone(
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  ScopedWorkOnTask work_on_task(this);

  if (status < 200 || status > 299) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  if (auto offchain_lookup = ExtractOffchainLookup(body)) {
    offchain_lookup_data_ = std::move(offchain_lookup);
    return;
  }

  auto bytes_result = ParseDecodedBytesResult(body);
  if (!bytes_result) {
    ParseErrorResult<mojom::ProviderError>(body, &error_.emplace(),
                                           &error_message_.emplace());
    return;
  }

  // Decoding as returned bytes[] per
  // https://github.com/ensdomains/docs/blob/e4da40003943dd25fdf7d4c5552335330a9ee915/ens-improvement-proposals/ensip-10-wildcard-resolution.md?plain=1#L70
  auto decoded_resolve_result = ExtractBytesFromTuple(*bytes_result, 0);
  if (!decoded_resolve_result) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  resolve_result_ = std::move(*decoded_resolve_result);
}

void EnsGetEthAddrTask::FetchOffchainData() {
  DCHECK(offchain_lookup_data_);

  GURL offchain_url;
  bool data_substitued = false;
  bool valid_sender = true;

  // Sender must match resolver per
  // https://eips.ethereum.org/EIPS/eip-3668#client-lookup-protocol #5.
  if (base::CompareCaseInsensitiveASCII(offchain_lookup_data_->sender.ToHex(),
                                        *resolver_)) {
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
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&EnsGetEthAddrTask::WorkOnTask,
                                  weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  std::string payload;
  if (!data_substitued) {
    base::Value::Dict payload_dict;
    payload_dict.Set("sender", offchain_lookup_data_->sender.ToHex());
    payload_dict.Set("data", ToHex(offchain_lookup_data_->call_data));
    base::JSONWriter::Write(payload_dict, &payload);
  }

  url_loader_ = CreateLoader(offchain_url, payload);
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&EnsGetEthAddrTask::OnFetchOffchainDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EnsGetEthAddrTask::OnFetchOffchainDone(
    std::unique_ptr<std::string> response_body) {
  ScopedWorkOnTask work_on_task(this);

  auto status = -1;

  if (url_loader_->ResponseInfo()) {
    auto headers_list = url_loader_->ResponseInfo()->headers;
    if (headers_list) {
      status = headers_list->response_code();
    }
  }

  if (status < 200 || status > 299 || !response_body) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  auto bytes_result = ExtractGatewayResult(*response_body);
  if (!bytes_result) {
    error_ = mojom::ProviderError::kInternalError;
    error_message_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return;
  }

  offchain_lookup_attemps_left_--;
  DCHECK_GE(offchain_lookup_attemps_left_, 0);
  ens_resolve_call_ =
      EncodeCall(offchain_lookup_data_->callback_function, *bytes_result,
                 offchain_lookup_data_->extra_data);
  offchain_lookup_data_.reset();
}

}  // namespace brave_wallet
