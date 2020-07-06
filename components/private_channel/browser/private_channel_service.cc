/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/private_channel/browser/private_channel_service.h"
#include "brave/components/private_channel/client_private_channel.h"

#include "base/logging.h"
#include "extensions/common/url_pattern.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"

const int kMaxPrivateChannelServerResponseSizeBytes = 1024 * 1024;

// @gpestana(TODO: refactor const to static values) 
const std::string metaUrl = "http://0.0.0.0:80/v1/meta";
const std::string firstRoundUrl = "http://0.0.0.0:80/v1/attestation/start";
const std::string secondRoundUrl = "http://0.0.0.0:80/v1/attestation/result";

// @gpestana(TODO: refactor to static values)
static const uint8_t PRIVATE_CHANNEL_SERVER_PK[] = {
  250, 180, 202, 58, 178, 175, 23, 9, 183, 97, 62, 167, 202,
  142, 210, 41, 122, 118, 82, 204, 98, 71, 134, 215, 67, 15,
  128, 124, 217, 120, 172, 93};

namespace brave_private_channel {

  // @gpestana(TODO: do we need anything to pass as constructor?)
  PrivateChannel::PrivateChannel(){
  }

  PrivateChannel::~PrivateChannel(){
  }

  void PrivateChannel::PerformReferralAttestation() {
     LOG(INFO) << "PrivateChannel::PerformReferralAttestation";

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->method = "GET";
    resource_request->url = GURL(metaUrl);
    resource_request->headers.SetHeader("Content-Type", "application/x-www-form-urlencoded");
    resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

    network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()->GetURLLoaderFactory();

    net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_private_channel_meta", R"(
        semantics {
          sender:
            "Brave Private Channel Service"
          description:
            "Requests Metadata from Private Channel Server"
            "to setup private two-party computation channel."
          trigger:
            "When starting a new attestation based on Private Channels"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");

    http_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
    http_loader_->SetAllowHttpErrorResults(true);

    http_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&PrivateChannel::OnPrivateChannelMetaLoadComplete,
                      base::Unretained(this)),
                      kMaxPrivateChannelServerResponseSizeBytes);
  }

  void PrivateChannel::OnPrivateChannelMetaLoadComplete(
      std::unique_ptr<std::string> response_body) {
        int response_code = -1;
        if (http_loader_->ResponseInfo() &&
            http_loader_->ResponseInfo()->headers)
          response_code =
              http_loader_->ResponseInfo()->headers->response_code();

        const std::string safe_response_body =
          response_body ? *response_body : std::string();

        if (http_loader_->NetError() != net::OK || response_code < 200 ||
            response_code > 299) {
          LOG(ERROR) << "Failed to fetch metadata from private channel server"
                     << ", error: " << http_loader_->NetError()
                     << ", response code: " << response_code
                     << ", payload: " << safe_response_body
                     << ", url: " << http_loader_->GetFinalURL().spec();
          return;

        }
        LOG(INFO) << "PrivateChannel::OnPrivateChannelMetaLoadComplete: " << safe_response_body;
      
        this->FirstRoundProtocol(safe_response_body);
  }

  void PrivateChannel::FirstRoundProtocol(std::string server_pk) {
    LOG(INFO) << "PrivateChannel::FirstRoundProtocol";

    // TODO(gpestana): refactor and get signals
    std::string sig0 = "check1";
    std::string sig1 = "check2";
    std::string sig2 = "check3";

    const char* input[] = { sig0.c_str(), sig1.c_str(), sig2.c_str() };
    int input_size = sizeof(input)/sizeof(input[0]);

    auto request_artefacts = ChallengeFirstRound(input, input_size, PRIVATE_CHANNEL_SERVER_PK);
    
    // TODO(gpestana): get referral id (most likely from the private 
    // channel constructor)    
    std::string id = "ref_id";

    const std::string payload = base::StringPrintf(
      "pk=%s&th_key=%s&enc_signals=%s&client_id=%s",
      request_artefacts.client_pk.c_str(),
      request_artefacts.shared_pubkey.c_str(),
      request_artefacts.encrypted_hashes.c_str(),
      id.c_str());

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->method = "POST";
    resource_request->url = GURL(firstRoundUrl);
    std::string content_type = "application/x-www-form-urlencoded";
    resource_request->headers.SetHeader("Content-Type", content_type);
    resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

    network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()->GetURLLoaderFactory();

    net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_private_channel_first_round", R"(
        semantics {
          sender:
            "Brave Private Channel Service"
          description:
            "Runs first round of Private Channel protocol"
          trigger:
            "When running attestation based on Private Channels"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");

    http_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
    http_loader_->SetAllowHttpErrorResults(true);
    http_loader_->AttachStringForUpload(payload, content_type);
    http_loader_->DownloadToString(
      loader_factory, base::BindOnce(
        &PrivateChannel::OnPrivateChannelFirstRoundLoadComplete,
          base::Unretained(this),
          request_artefacts.client_sk,
          id,
          input_size),
        kMaxPrivateChannelServerResponseSizeBytes);
  }

  void PrivateChannel::OnPrivateChannelFirstRoundLoadComplete(
      std::string client_sk,
      std::string id,
      int input_size,
      std::unique_ptr<std::string> response_body) {

        LOG(INFO) << "PrivateChannel::OnPrivateChannelFirstRoundLoadComplete";

        int response_code = -1;
        if (http_loader_->ResponseInfo() &&
            http_loader_->ResponseInfo()->headers)
          response_code =
              http_loader_->ResponseInfo()->headers->response_code();

        const std::string safe_response_body =
          response_body ? *response_body : std::string();

        if (http_loader_->NetError() != net::OK || response_code < 200 ||
            response_code > 299) {
          LOG(ERROR) << "Failed run the first round of the private channels protocol"
              << ", error: " << http_loader_->NetError()
              << ", response code: " << response_code
              << ", payload: " << safe_response_body
              << ", url: " << http_loader_->GetFinalURL().spec();
          return;

        }
        LOG(INFO) << "PrivateChannel::OnPrivateChannelFirstRoundLoadComplete: " << safe_response_body;

        this->SecondRoundProtocol(
          safe_response_body.c_str(), client_sk, id, input_size);
  }

  void PrivateChannel::SecondRoundProtocol(
    const std::string& encrypted_input,
    std::string client_sk,
    std::string id,
    int input_size) {

    LOG(INFO) << "PrivateChannel::SecondRoundProtocol: " << encrypted_input;

    auto request_artefacts = SecondRound(
      encrypted_input.c_str(), input_size, &client_sk[0]);

    const std::string payload = base::StringPrintf(
      "rand_vec=%s&partial_dec=%s&proofs=%s&client_id=%s",
      request_artefacts.rand_vec.c_str(),
      request_artefacts.partial_decryption.c_str(),
      request_artefacts.proofs.c_str(),
      id.c_str());

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->method = "POST";
    resource_request->url = GURL(secondRoundUrl);
    std::string content_type = "application/x-www-form-urlencoded";
    resource_request->headers.SetHeader("Content-Type", content_type);
    resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

    network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()->GetURLLoaderFactory();

    net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_private_channel_second_round", R"(
        semantics {
          sender:
            "Brave Private Channel Service"
          description:
            "Runs second round of Private Channel protocol"
          trigger:
            "When running attestation based on Private Channels"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");

    http_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
    http_loader_->SetAllowHttpErrorResults(true);
    http_loader_->AttachStringForUpload(payload, content_type);
    http_loader_->DownloadToString(
      loader_factory, base::BindOnce(
        &PrivateChannel::OnPrivateChannelSecondRoundLoadComplete,
          base::Unretained(this)),
        kMaxPrivateChannelServerResponseSizeBytes);
  }

  void PrivateChannel::OnPrivateChannelSecondRoundLoadComplete(
      std::unique_ptr<std::string> response_body) {
        LOG(INFO) << "PrivateChannel::OnPrivateChannelSecondRoundLoadComplete";
  }

} // namespace 