/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/reporter.h"

#include <utility>

#include "base/containers/span_writer.h"
#include "base/json/json_writer.h"
#include "base/numerics/byte_conversions.h"
#include "base/rand_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/web_discovery/browser/ecdh_aes.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/signature_basename.h"
#include "brave/components/web_discovery/browser/util.h"
#include "crypto/sha2.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/zlib/google/compression_utils_portable.h"
#include "third_party/zlib/zlib.h"

namespace web_discovery {

namespace {

constexpr net::NetworkTrafficAnnotationTag kSubmitNetworkTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("wdp_submit", R"(
    semantics {
      sender: "Brave Web Discovery Submission"
      description:
        "Sends search engine results & page interaction metrics
        that are deemed private by risk assessment heuristics."
      trigger:
        "Requests are automatically sent every minute "
        "while Brave is running, and as content is collected."
      data: "Search engine results & page interaction metrics"
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can opt-in or out via brave://settings/search"
    })");

constexpr base::TimeDelta kRequestMaxAge = base::Hours(36);
constexpr base::TimeDelta kMinRequestInterval =
    base::Minutes(1) - base::Seconds(5);
constexpr base::TimeDelta kMaxRequestInterval =
    base::Minutes(1) + base::Seconds(5);
constexpr size_t kMaxRetries = 10;

constexpr char kTypeField[] = "type";
constexpr char kWdpType[] = "wdp";
constexpr char kChannelField[] = "channel";
constexpr char kBraveChannel[] = "brave";
constexpr char kReporterVersionField[] = "ver";
constexpr char kCurrentReporterVersion[] = "1.0";
constexpr char kAntiDuplicatesField[] = "anti-duplicates";
constexpr char kTimestampField[] = "ts";
constexpr int kMaxAntiDuplicatesNonce = 10000000;
constexpr char kSenderField[] = "sender";
constexpr char kHpnSenderValue[] = "hpnv2";

constexpr uint8_t kSignedMessageId = 0x03;
// id byte + basename count + signature
constexpr size_t kSignedMessageMetadataSize = 1 + 8 + 389;
constexpr size_t kMaxCompressedMessageSize = 32767;

constexpr char kSubmitPath[] = "/";
constexpr char kMessageContentType[] = "application/octet-stream";
constexpr char kKeyDateHeader[] = "Key-Date";
constexpr char kEncryptionHeader[] = "Encryption";

base::Value GenerateFinalPayload(const base::Value::Dict& pre_payload) {
  base::Value::Dict result = pre_payload.Clone();

  result.Set(kTypeField, kWdpType);
  result.Set(kReporterVersionField, kCurrentReporterVersion);
  result.Set(kSenderField, kHpnSenderValue);
  result.Set(kTimestampField, FormatServerDate(base::Time::Now()));
  result.Set(kAntiDuplicatesField, base::RandInt(0, kMaxAntiDuplicatesNonce));
  result.Set(kChannelField, kBraveChannel);

  return base::Value(std::move(result));
}

std::optional<CompressEncryptResult> CompressAndEncrypt(
    std::vector<uint8_t> full_signed_message,
    std::string server_pub_key) {
  uLongf compressed_data_size = compressBound(full_signed_message.size());
  std::vector<uint8_t> compressed_data(compressed_data_size + 2);
  if (zlib_internal::CompressHelper(
          zlib_internal::ZLIB, compressed_data.data(), &compressed_data_size,
          full_signed_message.data() + 2, full_signed_message.size(),
          Z_DEFAULT_COMPRESSION, nullptr, nullptr) != Z_OK) {
    return std::nullopt;
  }
  if (compressed_data_size > kMaxCompressedMessageSize) {
    return std::nullopt;
  }
  base::ranges::copy(base::U16ToBigEndian(compressed_data_size),
                     compressed_data.begin());
  auto encrypt_result = DeriveAESKeyAndEncrypt(server_pub_key, compressed_data);
  if (!encrypt_result) {
    return std::nullopt;
  }
  return std::make_optional<CompressEncryptResult>(
      compressed_data, encrypt_result->encoded_public_component_and_iv);
}

}  // namespace

CompressEncryptResult::CompressEncryptResult(std::vector<uint8_t> data,
                                             std::string encoded)
    : encrypted_data(data), encoded_public_component_and_iv(encoded) {}

CompressEncryptResult::~CompressEncryptResult() = default;
CompressEncryptResult::CompressEncryptResult(const CompressEncryptResult&) =
    default;

Reporter::Reporter(PrefService* profile_prefs,
                   network::SharedURLLoaderFactory* shared_url_loader_factory,
                   CredentialManager* credential_manager,
                   std::unique_ptr<ServerConfig>* last_loaded_server_config)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory),
      last_loaded_server_config_(last_loaded_server_config),
      credential_manager_(credential_manager),
      pool_sequenced_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({})),
      request_queue_(profile_prefs,
                     kScheduledReports,
                     kRequestMaxAge,
                     kMinRequestInterval,
                     kMaxRequestInterval,
                     kMaxRetries,
                     base::BindRepeating(&Reporter::PrepareRequest,
                                         base::Unretained(this))) {
  submit_url_ = GURL(GetCollectorHost() + kSubmitPath);
}

Reporter::~Reporter() = default;

void Reporter::ScheduleSend(base::Value::Dict payload) {
  request_queue_.ScheduleRequest(base::Value(std::move(payload)));
}

void Reporter::PrepareRequest(const base::Value& request_data) {
  if (!credential_manager_->CredentialExistsForToday()) {
    // Backoff until credential is available to today
    request_queue_.NotifyRequestComplete(false);
    return;
  }
  const auto* payload_dict = request_data.GetIfDict();
  if (!payload_dict) {
    // Drop request due to bad data
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  auto basename_result = GenerateBasename(
      profile_prefs_, (*last_loaded_server_config_).get(), *payload_dict);
  if (!basename_result) {
    // Drop request due to exceeded basename quota
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  auto final_payload = GenerateFinalPayload(*payload_dict);

  std::string final_payload_json;
  if (base::JSONWriter::Write(final_payload, &final_payload_json)) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }

  auto payload_hash = crypto::SHA256HashString(final_payload_json);
  credential_manager_->Sign(
      std::vector<const uint8_t>(payload_hash.begin(), payload_hash.end()),
      basename_result->basename,
      base::BindOnce(&Reporter::OnRequestSigned, weak_ptr_factory_.GetWeakPtr(),
                     final_payload_json, basename_result->count));
}

void Reporter::OnRequestSigned(
    std::string final_payload_json,
    size_t basename_count,
    std::optional<std::vector<const uint8_t>> signature) {
  if (!signature) {
    request_queue_.NotifyRequestComplete(false);
    return;
  }
  auto pub_key = (*last_loaded_server_config_)
                     ->pub_keys.find(FormatServerDate(base::Time::Now()));
  if (pub_key == (*last_loaded_server_config_)->pub_keys.end()) {
    request_queue_.NotifyRequestComplete(false);
    return;
  }
  std::vector<uint8_t> full_signed_message(kSignedMessageMetadataSize +
                                           final_payload_json.size());
  base::SpanWriter<uint8_t> message_writer(full_signed_message);
  if (!message_writer.WriteU8BigEndian(kSignedMessageId) ||
      !message_writer.Write(base::span<uint8_t>(
          reinterpret_cast<uint8_t*>(final_payload_json.data()),
          final_payload_json.size())) ||
      !message_writer.Write(base::DoubleToBigEndian(basename_count)) ||
      !message_writer.Write(*signature)) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&CompressAndEncrypt, full_signed_message, pub_key->second),
      base::BindOnce(&Reporter::OnRequestCompressedAndEncrypted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void Reporter::OnRequestCompressedAndEncrypted(
    std::optional<CompressEncryptResult> result) {
  if (!result) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  auto request = CreateResourceRequest(submit_url_);
  request->headers.SetHeader(kKeyDateHeader,
                             FormatServerDate(base::Time::Now()));
  request->headers.SetHeader(kEncryptionHeader,
                             result->encoded_public_component_and_iv);

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), kSubmitNetworkTrafficAnnotation);
  url_loader_->AttachStringForUpload(
      std::string(result->encrypted_data.begin(), result->encrypted_data.end()),
      kMessageContentType);
}

void Reporter::OnRequestComplete(std::optional<std::string> response_body) {
  request_queue_.NotifyRequestComplete(ValidateResponse(response_body));
}

bool Reporter::ValidateResponse(
    const std::optional<std::string>& response_body) {
  auto* response_info = url_loader_->ResponseInfo();
  if (!response_body || !response_info) {
    return false;
  }
  auto response_code = response_info->headers->response_code();
  if (response_code < 200 || response_code >= 300) {
    if (response_code >= 500) {
      // Only retry failures due to server error
      return false;
    }
  }
  return true;
}

}  // namespace web_discovery
