/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/communication_adapter.h"

#include "brave/components/brave_federated/task/communication_helper.h"
#include "brave/components/brave_federated/task/typing.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

#include <utility>
#include <vector>

#include <iostream>

namespace brave_federated {

namespace {

constexpr char kServerEndpoint[] = "https://fl.brave.com";

// TODO(lminto): update this annotation
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("federated_learning", R"(
        semantics {
          sender: "Federated Learning"
          description:
            "Report of anonymized engagement statistics. For more info see "
          trigger:
            "Reports are automatically generated on startup and at intervals "
            "while Brave is running."
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
    : url_loader_factory_(url_loader_factory) {}

void CommunicationAdapter::GetTasks(const GetTaskCallback& callback) {
  std::cerr << "**: Getting tasks..." << std::endl;
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kServerEndpoint);
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Requesting Tasks list " << request->method << " " << request->url;

  const std::string payload = BuildGetTasksPayload();

  VLOG(2) << "Payload " << payload;

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/protobuf");
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnGetTasks, base::Unretained(this),
                     std::move(callback)));
}

void CommunicationAdapter::OnGetTasks(
    const GetTaskCallback& callback,
    const std::unique_ptr<std::string> response_body) {
  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    VLOG(1) << "Failed to get federated tasks";
    return;
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    // TODO(lminto): Actually parse GetTasksResponse
    TaskList task_list;
    Task task_1 = Task(0, TaskType::TrainingTask);
    task_list.push_back(task_1);

    std::move(callback).Run(task_list);
    return;
  }

  VLOG(1) << "Failed to get tasks" << response_code;
}

void CommunicationAdapter::PostTaskResult(TaskResult result,
                                          const PostResultCallback& callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kServerEndpoint);
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Posting Task results " << request->method << " " << request->url;

  const std::string payload = BuildPostTaskResultsPayload();

  VLOG(2) << "Payload " << payload;

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/protobuf");

  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnPostTaskResult,
                     base::Unretained(this), callback));
}

void CommunicationAdapter::OnPostTaskResult(
    const PostResultCallback& callback,
    std::unique_ptr<std::string> response_body) {
  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    VLOG(1) << "Failed to get federated tasks";
    return;
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    TaskResultResponse response(/*successful*/ true);
    std::move(callback).Run(response);
    return;
  }

  VLOG(1) << "Failed to get tasks" << response_code;
}

}  // namespace brave_federated
