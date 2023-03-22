/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/communication_adapter.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_federated/adapters/flower_helper.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/task/typing.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace brave_federated {

namespace {

// TODO(lminto): update this annotation
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("federated_learning", R"(
        semantics {
          sender: "Federated Learning"
          description:
            "Report of anonymized engagement statistics. For more info see "
          trigger:
            "Reports are automatically generated on startup and at intervals
            " "while Brave is running."
          data:
            "Anonymized and encrypted engagement data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This service is enabled only when opted in to ads and having "
            "P3A is enabled."
          policy_exception_justification:
            "Not implemented."
        }
    )");
}

}  // namespace

CommunicationAdapter::CommunicationAdapter(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {
  reconnect_policy_ = std::make_unique<const net::BackoffEntry::Policy>(
      /*.num_errors_to_ignore = */ 0,
      /*.initial_delay_ms = */ 10 * 1000,
      /*.multiply_factor =*/2.0,
      /*.jitter_factor =*/0.0,
      /*.maximum_backoff_ms =*/60 * 10 * 1000,
      /*.always_use_initial_delay =*/true);
  reconnect_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(reconnect_policy_.get());

  request_task_policy_ = std::make_unique<const net::BackoffEntry::Policy>(
      /*.num_errors_to_ignore = */ 0,
      /*.initial_delay_ms = */
      features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
      /*.multiply_factor =*/2.0,
      /*.jitter_factor =*/0.0,
      /*.maximum_backoff_ms =*/16 *
          features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
      /*.always_use_initial_delay =*/true);
  request_task_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(request_task_policy_.get());
}

CommunicationAdapter::~CommunicationAdapter() = default;

void CommunicationAdapter::GetTasks(GetTaskCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(features::GetFederatedLearningTaskEndpoint());
  request->headers.SetHeader("Content-Type", "application/protobuf");
  request->headers.SetHeader("Accept", "application/protobuf");
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Requesting tasks list " << request->method << " " << request->url;

  const std::string& payload = BuildGetTasksPayload();

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/protobuf");
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnGetTasks, base::Unretained(this),
                     std::move(callback)));
}

void CommunicationAdapter::OnGetTasks(
    GetTaskCallback callback,
    const std::unique_ptr<std::string> response_body) {
  bool failed_request =
      !url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers;
  reconnect_backoff_entry_->InformOfRequest(!failed_request);
  int64_t reconnect_delay_in_seconds =
      reconnect_backoff_entry_->GetTimeUntilRelease().InSeconds();

  if (failed_request) {
    VLOG(1) << "Failed to request tasks, retrying in "
            << reconnect_delay_in_seconds << "s";
    std::move(callback).Run({}, reconnect_delay_in_seconds);
    return;
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  // TODO(lminto): Check headers (as per Daniel instructions)
  if (response_code == net::HTTP_OK) {
    TaskList task_list = ParseTaskListFromResponseBody(*response_body);
    bool empty_task_list = task_list.empty();
    VLOG(2) << "Received " << task_list.size() << " tasks from FL service";

    request_task_backoff_entry_->InformOfRequest(!empty_task_list);
    int64_t request_task_delay_in_seconds =
        request_task_backoff_entry_->GetTimeUntilRelease().InSeconds();
    if (empty_task_list) {
      VLOG(1) << "No tasks received from FL service, retrying in "
              << request_task_delay_in_seconds << "s";
      std::move(callback).Run({}, request_task_delay_in_seconds);
      return;
    }

    std::move(callback).Run(task_list, request_task_delay_in_seconds);
    return;
  }

  VLOG(1) << "Failed to request tasks. Response code: " << response_code;
}

void CommunicationAdapter::PostTaskResult(TaskResult result,
                                          PostResultCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(features::GetFederatedLearningResultsEndpoint());
  request->headers.SetHeader("Content-Type", "application/protobuf");
  request->headers.SetHeader("Accept", "application/protobuf");
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Posting Task results " << request->method << " " << request->url;

  const std::string& payload = BuildPostTaskResultsPayload(result);

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/protobuf");
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnPostTaskResult,
                     base::Unretained(this), std::move(callback)));
}

void CommunicationAdapter::OnPostTaskResult(
    PostResultCallback callback,
    std::unique_ptr<std::string> response_body) {
  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    VLOG(1) << "Failed to post task results";
    return;
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    TaskResultResponse response(/*successful*/ true);
    std::move(callback).Run(response);
    return;
  }

  VLOG(1) << "Failed to post task results" << response_code;
}

}  // namespace brave_federated
