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

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("federated_learning", R"(
        semantics {
          sender: "Federated Learning"
          description:
            "Federated model updates based off toy task and locally generated "
            "synthetic dataset."
          trigger:
            "Reports are generated when the brave-federated flag is enabled "
            "and learning tasks are made available by the federated server. "
            "Reports are NOT generated when the user is on battery power "
            "or when the device is on a metered network (not WiFi/Ethernet). "
          data:
            "Simple federated model updates based off toy task and locally "
            "generated synthetic data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This experimental feature is only enabled in Nightly builds and"
            "can be disabled via the 'brave://flags/#brave-federated' "
          policy_exception_justification:
            "Not implemented."
        }
    )");
}

}  // namespace

CommunicationAdapter::CommunicationAdapter(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const net::BackoffEntry::Policy reconnect_policy,
    const net::BackoffEntry::Policy request_task_policy)
    : url_loader_factory_(std::move(url_loader_factory)) {
  reconnect_policy_ = std::make_unique<const net::BackoffEntry::Policy>(reconnect_policy);
  reconnect_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(reconnect_policy_.get());

  request_task_policy_ = std::make_unique<const net::BackoffEntry::Policy>(
      request_task_policy);
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
      base::BindOnce(&CommunicationAdapter::OnGetTasks,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
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
    return std::move(callback).Run({}, reconnect_delay_in_seconds);
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK &&
      headers->HasHeaderValue("Content-Type", "application/protobuf")) {
    TaskList task_list = ParseTaskListFromResponseBody(*response_body);
    bool empty_task_list = task_list.empty();
    VLOG(2) << "Received " << task_list.size() << " tasks from FL service";

    request_task_backoff_entry_->InformOfRequest(!empty_task_list);
    int64_t request_task_delay_in_seconds =
        request_task_backoff_entry_->GetTimeUntilRelease().InSeconds();
    if (empty_task_list) {
      VLOG(1) << "No tasks received from FL service, retrying in "
              << request_task_delay_in_seconds << "s";
      return std::move(callback).Run({}, request_task_delay_in_seconds);
    }

    return std::move(callback).Run(task_list, request_task_delay_in_seconds);
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
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void CommunicationAdapter::OnPostTaskResult(
    PostResultCallback callback,
    std::unique_ptr<std::string> response_body) {
  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    VLOG(1) << "Failed to post task results";
    TaskResultResponse response(/*successful*/ false);
    return std::move(callback).Run(response);
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    TaskResultResponse response(/*successful*/ true);
    return std::move(callback).Run(response);
  }

  VLOG(1) << "Response code is not 200" << response_code;
  TaskResultResponse response(/*successful*/ false);
  return std::move(callback).Run(response);
}

}  // namespace brave_federated
