/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/communication_adapter.h"

#include "base/check.h"
#include "brave/components/brave_federated/task/communication_helper.h"
#include "brave/components/brave_federated/task/typing.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

#include <utility>
#include <vector>

#include <iostream>

namespace brave_federated {

namespace {

constexpr char kServerEndpoint[] = "https://fl.brave.com";
// constexpr char kLocalServerTasksEndpoint[] =
//     "http://127.0.0.1:8000/api/1.1/tasks";
// constexpr char kLocalServerResultsEndpoint[] =
//     "http://127.0.0.1:8000/api/1.1/results";

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
    : url_loader_factory_(url_loader_factory) {}

CommunicationAdapter::~CommunicationAdapter() = default;

void CommunicationAdapter::GetTasks(GetTaskCallback callback) {
  std::cerr << "**: Getting tasks..." << std::endl;
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kServerEndpoint);
  request->headers.SetHeader("Content-Type", "application/protobuf");
  request->headers.SetHeader("Accept", "application/protobuf");
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Requesting Tasks list " << request->method << " " << request->url;
  std::cerr << "Requesting Tasks list " << request->method << " "
            << request->url << std::endl;

  const std::string payload = BuildGetTasksPayload();

  VLOG(2) << "Payload " << payload;

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/protobuf");
  // url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
  //     url_loader_factory_.get(),
  //     base::BindOnce(&CommunicationAdapter::OnGetTasks,
  //                    base::Unretained(this), std::move(callback)));
  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&CommunicationAdapter::OnGetTasksHeader,
                     base::Unretained(this)));
}

void CommunicationAdapter::OnGetTasksHeader(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  if (!headers) {
    std::cerr << "**: Failed to send." << std::endl;
    return;
  }

  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
  }

  std::cerr << "**: Bad response..." << std::endl;
}

void CommunicationAdapter::OnGetTasks(
    GetTaskCallback callback,
    const std::unique_ptr<std::string> response_body) {
  std::cerr << "**: response body " << response_body << std::endl;
  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    std::cerr << "**: Failed to get tasks (no headers)" << std::endl;
    VLOG(1) << "Failed to get federated tasks";
    return;
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    // TODO(lminto): Actually parse GetTasksResponse
    std::cerr << "**: Succesfully got tasks" << std::endl;
    TaskList task_list;
    Task task_1 = Task(0, TaskType::TrainingTask);
    task_list.push_back(task_1);

    std::move(callback).Run(task_list);
    return;
  }

  std::cerr << "**: Failed to get tasks (bad response code)" << std::endl;
  VLOG(1) << "Failed to get tasks" << response_code;
}

void CommunicationAdapter::PostTaskResult(TaskResult result,
                                          PostResultCallback callback) {
  // TODO(lminto): shortcircuit network for now
  TaskResultResponse response(true);
  std::move(callback).Run(std::move(response));

  return;

  // auto request = std::make_unique<network::ResourceRequest>();
  // request->url = GURL(kServerEndpoint);
  // request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  // request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  // request->method = net::HttpRequestHeaders::kPostMethod;

  // VLOG(2) << "Posting Task results " << request->method << " " <<
  // request->url;

  // const std::string payload = BuildPostTaskResultsPayload();

  // VLOG(2) << "Payload " << payload;

  // url_loader_ = network::SimpleURLLoader::Create(
  //     std::move(request), GetNetworkTrafficAnnotationTag());
  // url_loader_->AttachStringForUpload(payload, "application/protobuf");

  // url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
  //     url_loader_factory_.get(),
  //     base::BindOnce(&CommunicationAdapter::OnPostTaskResult,
  //                    base::Unretained(this), callback));
}

void CommunicationAdapter::OnPostTaskResult(
    PostResultCallback callback,
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
