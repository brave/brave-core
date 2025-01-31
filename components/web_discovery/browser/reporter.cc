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
#include "base/threading/thread_restrictions.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/signature_basename.h"
#include "brave/components/web_discovery/browser/util.h"
#include "crypto/sha2.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
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

constexpr char kBraveChannel[] =
#if BUILDFLAG(IS_ANDROID)
    "brave-native-android";
#elif BUILDFLAG(IS_IOS)
    "brave-native-ios";
#else
    "brave-native-desktop";
#endif

constexpr char kReporterVersionField[] = "ver";
constexpr char kCurrentReporterVersion[] = "1.0";
constexpr char kAntiDuplicatesField[] = "anti-duplicates";
constexpr char kTimestampField[] = "ts";
constexpr int kMaxAntiDuplicatesNonce = 10000000;
constexpr char kSenderField[] = "sender";
constexpr char kHpnSenderValue[] = "hpnv2";

constexpr uint8_t kSignedMessageId = 0x03;
constexpr uint8_t kCompressedMessageId = 0x80;
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

std::optional<AESEncryptResult> CompressAndEncrypt(
    std::vector<uint8_t> full_signed_message,
    std::vector<uint8_t> server_pub_key) {
  base::AssertLongCPUWorkAllowed();
  uLongf compressed_data_size = compressBound(full_signed_message.size());
  std::vector<uint8_t> compressed_data(compressed_data_size + 2);
  if (zlib_internal::CompressHelper(
          zlib_internal::ZLIB,
          base::span(compressed_data).last(compressed_data_size).data(),
          &compressed_data_size, full_signed_message.data(),
          full_signed_message.size(), Z_DEFAULT_COMPRESSION, nullptr,
          nullptr) != Z_OK) {
    VLOG(1) << "Failed to compress payload";
    return std::nullopt;
  }
  compressed_data.resize(compressed_data_size + 2);
  if (compressed_data_size > kMaxCompressedMessageSize) {
    VLOG(1) << "Compressed payload exceeds limit of "
            << kMaxCompressedMessageSize << " bytes";
    return std::nullopt;
  }
  base::ranges::copy(base::U16ToBigEndian(compressed_data_size),
                     compressed_data.data());
  compressed_data[0] |= kCompressedMessageId;
  return DeriveAESKeyAndEncrypt(server_pub_key, compressed_data);
}

}  // namespace

Reporter::Reporter(PrefService* profile_prefs,
                   network::SharedURLLoaderFactory* shared_url_loader_factory,
                   CredentialSigner* credential_signer,
                   const ServerConfigLoader* server_config_loader)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory),
      credential_signer_(credential_signer),
      server_config_loader_(server_config_loader),
      request_queue_(profile_prefs,
                     kScheduledReports,
                     kRequestMaxAge,
                     kMinRequestInterval,
                     kMaxRequestInterval,
                     kMaxRetries,
                     base::BindRepeating(&Reporter::PrepareRequest,
                                         base::Unretained(this))) {
  submit_url_ = GURL(GetAnonymousHPNHost() + kSubmitPath);
}

Reporter::~Reporter() = default;

void Reporter::ScheduleSend(base::Value::Dict payload) {
  request_queue_.ScheduleRequest(std::move(payload));
}

void Reporter::PrepareRequest(const base::Value& request_data) {
  VLOG(1) << "Preparing request";
  if (!credential_signer_->CredentialExistsForToday()) {
    // Backoff until credential is available to today
    VLOG(1) << "Credential does not exist for today";
    request_queue_.NotifyRequestComplete(false);
    return;
  }
  const auto* payload_dict = request_data.GetIfDict();
  if (!payload_dict) {
    // Drop request due to bad data
    VLOG(1) << "Payload is not a dictionary";
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  auto basename_result = GenerateBasename(
      profile_prefs_, server_config_loader_->GetLastServerConfig(),
      *payload_dict);
  if (!basename_result) {
    // Drop request due to exceeded basename quota
    VLOG(1) << "Failed to generate basename";
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  auto final_payload = GenerateFinalPayload(*payload_dict);

  std::string final_payload_json;
  if (!base::JSONWriter::Write(final_payload, &final_payload_json)) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }

  auto payload_hash = crypto::SHA256HashString(final_payload_json);
  credential_signer_->Sign(
      std::vector<uint8_t>(payload_hash.begin(), payload_hash.end()),
      basename_result->basename,
      base::BindOnce(&Reporter::OnRequestSigned, base::Unretained(this),
                     final_payload_json, basename_result->count_tag_hash,
                     basename_result->count));
}

void Reporter::OnRequestSigned(std::string final_payload_json,
                               uint32_t count_tag_hash,
                               size_t basename_count,
                               std::optional<std::vector<uint8_t>> signature) {
  if (!signature) {
    request_queue_.NotifyRequestComplete(false);
    return;
  }
  const auto& server_config = server_config_loader_->GetLastServerConfig();
  auto pub_key =
      server_config.pub_keys.find(FormatServerDate(base::Time::Now()));
  if (pub_key == server_config.pub_keys.end()) {
    VLOG(1) << "No ECDH server public key available";
    request_queue_.NotifyRequestComplete(false);
    return;
  }
  std::vector<uint8_t> full_signed_message(kSignedMessageMetadataSize +
                                           final_payload_json.size());
  base::SpanWriter<uint8_t> message_writer(full_signed_message);
  if (!message_writer.WriteU8BigEndian(kSignedMessageId) ||
      !message_writer.Write(std::vector<uint8_t>(final_payload_json.begin(),
                                                 final_payload_json.end())) ||
      !message_writer.Write(base::DoubleToBigEndian(basename_count)) ||
      !message_writer.Write(*signature)) {
    VLOG(1) << "Failed to pack signed message";
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&CompressAndEncrypt, full_signed_message, pub_key->second),
      base::BindOnce(&Reporter::OnRequestCompressedAndEncrypted,
                     weak_ptr_factory_.GetWeakPtr(), count_tag_hash,
                     basename_count));
}

void Reporter::OnRequestCompressedAndEncrypted(
    uint32_t count_tag_hash,
    size_t basename_count,
    std::optional<AESEncryptResult> result) {
  if (!result) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }
  auto request = CreateResourceRequest(submit_url_);
  request->method = net::HttpRequestHeaders::kPostMethod;
  request->headers.SetHeader(kKeyDateHeader,
                             FormatServerDate(base::Time::Now()));
  request->headers.SetHeader(kEncryptionHeader,
                             result->encoded_public_component_and_iv);
  request->headers.SetHeader(kVersionHeader,
                             base::NumberToString(kCurrentVersion));

  VLOG(1) << "Sending message";
  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), kSubmitNetworkTrafficAnnotation);
  url_loader_->AttachStringForUpload(
      std::string(result->data.begin(), result->data.end()),
      kMessageContentType);
  url_loader_->DownloadHeadersOnly(
      shared_url_loader_factory_.get(),
      base::BindOnce(&Reporter::OnRequestComplete, base::Unretained(this),
                     count_tag_hash, basename_count));
}

void Reporter::OnRequestComplete(
    uint32_t count_tag_hash,
    size_t basename_count,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  auto result = ValidateResponse(headers);
  VLOG(1) << "Submission result: " << result;
  if (result) {
    SaveBasenameCount(profile_prefs_, count_tag_hash, basename_count);
  }
  request_queue_.NotifyRequestComplete(result);
}

bool Reporter::ValidateResponse(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  if (!headers) {
    return false;
  }
  auto response_code = headers->response_code();
  if (!network::IsSuccessfulStatus(response_code)) {
    if (response_code >= net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR) {
      // Only retry failures due to server error
      return false;
    }
  }
  return true;
}

}  // namespace web_discovery
