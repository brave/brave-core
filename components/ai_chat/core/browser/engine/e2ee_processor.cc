// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/e2ee_processor.h"

#include <utility>

#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ai_chat {

namespace {

constexpr char kAttestationPathFormat[] = "v1/models/%s/attestation";
constexpr char kModelPublicKeyKey[] = "model_public_key";

constexpr base::TimeDelta kAttestationCacheTTL = base::Hours(1);

constexpr std::string_view kThinkOpen = "<think>";
constexpr std::string_view kThinkClose = "</think>";

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "Fetches the TEE model attestation for end-to-end encryption when "
          "communicating with Brave's AI Chat service."
        trigger:
          "Triggered when making an encrypted AI Chat request using a model "
          "that supports end-to-end encryption."
        data:
          "The model name is sent as part of the URL path. No user data is "
          "sent."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");

}  // namespace

E2EEProcessor::E2EEProcessor(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      kTrafficAnnotation, url_loader_factory);
}

E2EEProcessor::~E2EEProcessor() = default;

E2EEProcessor::ClientKeyPair::ClientKeyPair(
    std::string public_key_hex,
    ClientSecretKeyBox secret_key)
    : public_key_hex(std::move(public_key_hex)),
      secret_key(std::move(secret_key)) {}
E2EEProcessor::ClientKeyPair::~ClientKeyPair() = default;

E2EEProcessor::Attestation::Attestation(std::vector<uint8_t> model_public_key)
    : model_public_key(std::move(model_public_key)),
      cached_at(base::TimeTicks::Now()) {}
E2EEProcessor::Attestation::~Attestation() = default;

void E2EEProcessor::SetAPIRequestHelperForTesting(
    std::unique_ptr<api_request_helper::APIRequestHelper> api_helper) {
  api_request_helper_ = std::move(api_helper);
}

void E2EEProcessor::ClearCachedModelAttestations() {
  attestation_cache_.clear();
}

void E2EEProcessor::FetchModelAttestation(
    const std::string& model_name,
    FetchModelAttestationCallback callback) {
  if (const auto* cached = base::FindPtrOrNull(attestation_cache_, model_name)) {
    if (base::TimeTicks::Now() - cached->cached_at <=
        kAttestationCacheTTL) {
      std::move(callback).Run(std::nullopt);
      return;
    }
    attestation_cache_.erase(model_name);
  }

  const std::string path = absl::StrFormat(kAttestationPathFormat, model_name);
  const GURL url = GetEndpointUrl(false, path);

  api_request_helper_->Request(
      net::HttpRequestHeaders::kGetMethod, url,
      /*content=*/"", /*content_type=*/"",
      base::BindOnce(&E2EEProcessor::OnFetchModelAttestationComplete,
                     weak_ptr_factory_.GetWeakPtr(), model_name,
                     std::move(callback)),
      /*headers=*/{}, /*options=*/{});
}

void E2EEProcessor::OnFetchModelAttestationComplete(
    const std::string& model_name,
    FetchModelAttestationCallback callback,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode() || !result.value_body().is_dict()) {
    std::move(callback).Run(mojom::APIError::ConnectionIssue);
    return;
  }

  const std::string* model_public_key_hex =
      result.value_body().GetDict().FindString(kModelPublicKeyKey);
  if (!model_public_key_hex || model_public_key_hex->empty()) {
    std::move(callback).Run(mojom::APIError::InternalError);
    return;
  }

  std::vector<uint8_t> model_public_key_bytes;
  if (!base::HexStringToBytes(*model_public_key_hex, &model_public_key_bytes)) {
    std::move(callback).Run(mojom::APIError::InternalError);
    return;
  }

  attestation_cache_[model_name] =
      std::make_unique<Attestation>(std::move(model_public_key_bytes));
  std::move(callback).Run(std::nullopt);
}

E2EEProcessor::ClientKeyPair
E2EEProcessor::GenerateClientKeyPair() {
  auto result = ai_chat::generate_client_keypair();
  const std::string public_key_hex = base::HexEncodeLower(result.public_key);
  return ClientKeyPair(public_key_hex, std::move(result.secret_key));
}

E2EEProcessor::EncryptCallback E2EEProcessor::CreateEncryptCallback(
    const std::string& model_name) {
  const auto* cached_attestation =
      base::FindPtrOrNull(attestation_cache_, model_name);
  if (!cached_attestation) {
    return {};
  }

  const std::vector<uint8_t> model_public_key =
      cached_attestation->model_public_key;
  return base::BindRepeating(
      [](const std::vector<uint8_t>& model_public_key,
         const base::ListValue& content) -> std::optional<std::string> {
        std::string json;
        base::JSONWriter::Write(content, &json);
        LOG(ERROR) << "about to encrypt" << json;

        auto ciphertext = ai_chat::encrypt(json, model_public_key);

        if (ciphertext.empty()) {
          return std::nullopt;
        }

        return base::HexEncodeLower(ciphertext);
      },
      model_public_key);
}

E2EEProcessor::DecryptCallback E2EEProcessor::CreateDecryptCallback(
    const ClientSecretKey* key) {
  return base::BindRepeating(
      [](const ClientSecretKey* secret_key,
         const std::string& ciphertext_hex) -> std::optional<std::string> {
        std::string result;
        std::string_view remaining = ciphertext_hex;

        while (!remaining.empty()) {
          const size_t open_pos = remaining.find(kThinkOpen);
          const size_t close_pos = remaining.find(kThinkClose);
          const size_t tag_pos = std::min(open_pos, close_pos);

          const std::string_view segment = remaining.substr(0, tag_pos);
          if (!segment.empty()) {
            std::vector<uint8_t> bytes;
            if (!base::HexStringToBytes(segment, &bytes) || bytes.empty()) {
              // If the segment is not hex, assume it's already plaintext
              // i.e. a error generated by the aichat server
              result += segment;
            } else {
              auto plaintext = ai_chat::decrypt(bytes, *secret_key);
              if (plaintext.empty()) {
                return std::nullopt;
              }
              result += std::string(plaintext);
            }
          }

          if (tag_pos != std::string_view::npos) {
            const auto& tag = tag_pos == open_pos ? kThinkOpen : kThinkClose;
            result += tag;
            remaining.remove_prefix(tag_pos + tag.size());
          } else {
            break;
          }
        }

        return result;
      },
      key);
}

}  // namespace ai_chat
