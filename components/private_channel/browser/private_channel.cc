/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/private_channel/browser/private_channel.h"

#include <utility>

#include "brave/components/private_channel/browser/constants.h"
#include "brave/components/private_channel/browser/request_utils.h"
#include "brave/components/private_channel/client_private_channel.h"
#include "brave/components/private_channel/utils.h"

#include "base/logging.h"
#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace private_channel {

PrivateChannel::PrivateChannel()
    : task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock()})),
      weak_factory_(this) {}

PrivateChannel::~PrivateChannel() {}

void PrivateChannel::PerformReferralAttestation(std::string referral_code) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  VLOG(1) << "PrivateChannel::PerformReferralAttestation";

  referral_code_ = referral_code;
  FetchMetadataPrivateChannelServer();
}

void PrivateChannel::FetchMetadataPrivateChannelServer() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->method = "GET";
  resource_request->url = GURL(request_utils::BuildUrl(
      request_utils::EndpointType::META, PRIVATE_CHANNEL_API_VERSION));
  resource_request->headers.SetHeader("Content-Type",
                                      "application/x-www-form-urlencoded");
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();

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

  http_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                  traffic_annotation);

  http_loader_->SetAllowHttpErrorResults(true);

  http_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&PrivateChannel::OnPrivateChannelMetaLoadComplete,
                     base::Unretained(this)),
      kMaxPrivateChannelServerResponseSizeBytes);
}

void PrivateChannel::OnPrivateChannelMetaLoadComplete(
    std::unique_ptr<std::string> response_body) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  int response_code = -1;
  if (http_loader_->ResponseInfo() && http_loader_->ResponseInfo()->headers)
    response_code = http_loader_->ResponseInfo()->headers->response_code();

  const std::string safe_response_body =
      response_body ? *response_body : std::string();

  if (http_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    VLOG(0) << "Failed to fetch metadata from private channel server"
            << ", error: " << http_loader_->NetError()
            << ", response code: " << response_code
            << ", payload: " << safe_response_body
            << ", url: " << http_loader_->GetFinalURL().spec();
    http_loader_.reset();
    return;
  }
  http_loader_.reset();

  base::SequencedTaskRunnerHandle::Get()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ChallengeFirstRound, safe_response_body),
      base::BindOnce(&PrivateChannel::FirstRoundProtocol,
                     weak_factory_.GetWeakPtr()));
}

void PrivateChannel::FirstRoundProtocol(ChallengeArtefacts request_artefacts) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string payload = base::StringPrintf(
      "pk_vector=%s&th_key_vector=%s&enc_signals=%s&client_id=%s&"
      "version=%s",
      request_artefacts.client_pks.c_str(),
      request_artefacts.shared_pubkey.c_str(),
      request_artefacts.encrypted_hashes.c_str(), referral_code_.c_str(),
      kPrivateChannelVersion);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->method = "POST";
  resource_request->url = GURL(request_utils::BuildUrl(
      request_utils::EndpointType::FIRST_ROUND, PRIVATE_CHANNEL_API_VERSION));
  std::string content_type = "application/x-www-form-urlencoded";
  resource_request->headers.SetHeader("Content-Type", content_type);
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_private_channel_first_round",
                                          R"(
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

  http_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                  traffic_annotation);

  http_loader_->SetAllowHttpErrorResults(true);
  http_loader_->AttachStringForUpload(payload, content_type);
  http_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&PrivateChannel::OnPrivateChannelFirstRoundLoadComplete,
                     base::Unretained(this), request_artefacts.client_sks,
                     referral_code_, request_artefacts.encrypted_hashes_size),
      kMaxPrivateChannelServerResponseSizeBytes);
}

void PrivateChannel::OnPrivateChannelFirstRoundLoadComplete(
    std::string client_sks,
    std::string id,
    int encrypted_hashes_size,
    std::unique_ptr<std::string> response_body) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  int response_code = -1;
  if (http_loader_->ResponseInfo() && http_loader_->ResponseInfo()->headers)
    response_code = http_loader_->ResponseInfo()->headers->response_code();

  const std::string safe_response_body =
      response_body ? *response_body : std::string();

  if (http_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    VLOG(0) << "Failed to run the first round of the private channels protocol"
            << ", error: " << http_loader_->NetError()
            << ", response code: " << response_code
            << ", payload: " << safe_response_body
            << ", url: " << http_loader_->GetFinalURL().spec();
    http_loader_.reset();
    return;
  }

  http_loader_.reset();

  base::SequencedTaskRunnerHandle::Get()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&SecondRound, safe_response_body, encrypted_hashes_size,
                     client_sks),
      base::BindOnce(&PrivateChannel::SecondRoundProtocol,
                     weak_factory_.GetWeakPtr()));
}

void PrivateChannel::SecondRoundProtocol(
    SecondRoundArtefacts request_artefacts) {
  const std::string payload = base::StringPrintf(
      "rand_vec=%s&partial_dec=%s&rand_proofs=%s&dec_proofs=%s&client_id=%s",
      request_artefacts.rand_vec.c_str(),
      request_artefacts.partial_decryption.c_str(),
      request_artefacts.dec_proofs.c_str(), request_artefacts.proofs.c_str(),
      referral_code_.c_str());

  if (request_artefacts.error) {
    VLOG(0) << "SecondRoundProtocol error. Stopping protocol.";
    return;
  }

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->method = "POST";
  resource_request->url = GURL(request_utils::BuildUrl(
      request_utils::EndpointType::SECOND_ROUND, PRIVATE_CHANNEL_API_VERSION));
  std::string content_type = "application/x-www-form-urlencoded";
  resource_request->headers.SetHeader("Content-Type", content_type);
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_private_channel_second_round",
                                          R"(
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

  http_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                  traffic_annotation);
  http_loader_->SetAllowHttpErrorResults(true);
  http_loader_->AttachStringForUpload(payload, content_type);
  http_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&PrivateChannel::OnPrivateChannelSecondRoundLoadComplete,
                     base::Unretained(this)),
      kMaxPrivateChannelServerResponseSizeBytes);
}

void PrivateChannel::OnPrivateChannelSecondRoundLoadComplete(
    std::unique_ptr<std::string> response_body) {
  int response_code = -1;
  if (http_loader_->ResponseInfo() && http_loader_->ResponseInfo()->headers)
    response_code = http_loader_->ResponseInfo()->headers->response_code();

  const std::string safe_response_body =
      response_body ? *response_body : std::string();

  if (response_code < 200 || response_code > 299) {
    VLOG(0) << "Failed to run the second round of the private channels protocol"
            << ", error: " << http_loader_->NetError()
            << ", response code: " << response_code
            << ", payload: " << safe_response_body
            << ", url: " << http_loader_->GetFinalURL().spec();
    http_loader_.reset();
    return;
  }

  http_loader_.reset();
  VLOG(1) << "PrivateChannel: Protocol successful";
}

}  // namespace private_channel
