/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/communication_adapter.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/adapters/flower_helper.h"
#include "brave/components/brave_federated/task/typing.h"
#include "net/base/load_flags.h"
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

// Maximum size of the federated server response in bytes.
const size_t kMaxFederatedServerResponseSizeBytes = 1024 * 1024;  // 1 MB

std::unique_ptr<network::ResourceRequest> MakeResourceRequest() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->headers.SetHeader("Content-Type", "application/protobuf");
  request->headers.SetHeader("Accept", "application/protobuf");
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;
  return request;
}

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
            "This experimental feature is off by default. It can be enabled
            "by turning on the #brave-federated experimental flag found at "
            "'brave://flags/#brave-federated'.  "
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
  reconnect_policy_ =
      std::make_unique<const net::BackoffEntry::Policy>(reconnect_policy);
  reconnect_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(reconnect_policy_.get());

  request_task_policy_ =
      std::make_unique<const net::BackoffEntry::Policy>(request_task_policy);
  request_task_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(request_task_policy_.get());
}

CommunicationAdapter::~CommunicationAdapter() = default;

void CommunicationAdapter::GetTasks(GetTaskCallback callback) {
  auto request = MakeResourceRequest();
  VLOG(2) << "FL: Requesting tasks list " << request->method << " "
          << request->url;

  absl::optional<std::string> payload = BuildGetTasksPayload();
  if (!payload.has_value()) {
    return;
  }

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload.value(), "application/protobuf");
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnGetTasks,
                     weak_factory_.GetWeakPtr(), std::ref(callback)),
      kMaxFederatedServerResponseSizeBytes);
}

void CommunicationAdapter::UploadTaskResult(const TaskResult& result,
                                            UploadResultCallback callback) {
  auto request = MakeResourceRequest();
  VLOG(2) << "FL: Posting Task results " << request->method << " "
          << request->url;

  absl::optional<std::string> payload = BuildUploadTaskResultsPayload(result);
  if (!payload.has_value()) {
    return;
  }

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload.value(), "application/protobuf");
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnUploadTaskResult,
                     weak_factory_.GetWeakPtr(), std::ref(callback)),
      kMaxFederatedServerResponseSizeBytes);
}

void CommunicationAdapter::OnGetTasks(
    GetTaskCallback& callback,
    const std::unique_ptr<std::string> response_body) {
  CHECK(url_loader_);
  CHECK(url_loader_->ResponseInfo());

  const bool failed_request =
      !url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers;
  reconnect_backoff_entry_->InformOfRequest(!failed_request);
  base::TimeDelta reconnect_delay_in_seconds =
      reconnect_backoff_entry_->GetTimeUntilRelease();

  if (failed_request) {
    VLOG(2) << "FL: Failed to request tasks, retrying in "
            << reconnect_delay_in_seconds;
    return std::move(callback).Run({}, reconnect_delay_in_seconds);
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();

  if (response_code != net::HTTP_OK ||
      !headers->HasHeaderValue("Content-Type", "application/protobuf")) {
    VLOG(2) << "FL: Failed to request tasks. Response code: " << response_code;
    return std::move(callback).Run({}, reconnect_delay_in_seconds);
  }

  auto task_list = ParseTaskListFromResponseBody(*response_body);

  request_task_backoff_entry_->InformOfRequest(task_list.has_value());
  base::TimeDelta request_task_delay_in_seconds =
      request_task_backoff_entry_->GetTimeUntilRelease();
  if (!task_list.has_value()) {
    VLOG(2) << "FL: ParseTaskList failed. Reason: " << task_list.error()
            << ". Retrying in " << request_task_delay_in_seconds;
    return std::move(callback).Run({}, request_task_delay_in_seconds);
  }

  VLOG(2) << "FL: Received " << task_list.value().size()
          << " tasks from FL service";

  return std::move(callback).Run(task_list.value(),
                                 request_task_delay_in_seconds);
}

void CommunicationAdapter::OnUploadTaskResult(
    UploadResultCallback& callback,
    std::unique_ptr<std::string> response_body) {
  CHECK(url_loader_);
  CHECK(url_loader_->ResponseInfo());

  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    VLOG(2) << "FL: Failed to post task results";
    TaskResultResponse response(/*successful*/ false);
    return std::move(callback).Run(response);
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();

  if (response_code != net::HTTP_OK) {
    VLOG(2) << "FL: Failed to post task results. Response code: "
            << response_code;
    TaskResultResponse response(/*successful*/ false);
    return std::move(callback).Run(response);
  }

  TaskResultResponse response(/*successful*/ true);
  return std::move(callback).Run(response);
}

}  // namespace brave_federated
