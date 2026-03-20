// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/e2ee_processor.h"

#include <utility>

#include "base/containers/map_util.h"
#include "base/functional/bind.h"
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
  if (const auto* cached = base::FindOrNull(attestation_cache_, model_name)) {
    if (base::TimeTicks::Now() - cached->cached_at <= kAttestationCacheTTL) {
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

  const std::string* model_public_key =
      result.value_body().GetDict().FindString(kModelPublicKeyKey);
  if (!model_public_key || model_public_key->empty()) {
    std::move(callback).Run(mojom::APIError::InternalError);
    return;
  }

  attestation_cache_[model_name] =
      Attestation{*model_public_key, base::TimeTicks::Now()};
  std::move(callback).Run(std::nullopt);
}

}  // namespace ai_chat
