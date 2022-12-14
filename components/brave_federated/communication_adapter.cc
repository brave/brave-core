/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/communication_adapter.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_federated/task/communication_helper.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/task.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"
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

// constexpr char kServerEndpoint[] = "https://fl.brave.com";
constexpr char kLocalServerTasksEndpoint[] =
    "http://127.0.0.1:8000/api/1.1/tasks";
constexpr char kLocalServerResultsEndpoint[] =
    "http://127.0.0.1:8000/api/1.1/results";

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
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kLocalServerTasksEndpoint);
  request->headers.SetHeader("Content-Type", "application/protobuf");
  request->headers.SetHeader("Accept", "application/protobuf");
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Requesting Tasks list " << request->method << " " << request->url;

  const std::string& payload = BuildGetTasksPayload();

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
    GetTaskCallback callback,
    const std::unique_ptr<std::string> response_body) {
  if (!url_loader_->ResponseInfo() || !url_loader_->ResponseInfo()->headers) {
    std::move(callback).Run({}, 10);
    VLOG(1) << "Failed to get federated tasks";
    return;
  }

  auto headers = url_loader_->ResponseInfo()->headers;
  int response_code = headers->response_code();
  // TODO(lminto): Check headers (as per Daniel instructions)
  if (response_code == net::HTTP_OK) {
    flower::GetTasksResponse task_response;
    if (task_response.ParseFromString(*response_body)) {
      if (task_response.has_tokenized_tasks()) {
        flower::TokenizedTasks tok_tasks = task_response.tokenized_tasks();

        TaskList task_list;
        for (int i = 0; i < tok_tasks.tokenized_tasks_size(); i++) {
          flower::TokenizedTask tok_task = tok_tasks.tokenized_tasks(i);

          std::string token = tok_task.token();
          flower::Task flwr_task = tok_task.task();

          int task_id = flwr_task.task_id();
          flower::ServerMessage message = flwr_task.legacy_server_message();

          TaskType type;
          if (message.has_fit_ins()) {
            type = TaskType::Training;
          } else if (message.has_evaluate_ins()) {
            type = TaskType::Evaluation;
          } else if (message.has_reconnect_ins()) {
            std::move(callback).Run(task_list,
                                    message.reconnect_ins().seconds());
            return;
          } else {
            // Unrecognized instruction
            VLOG(2) << "**: Received unrecognized instruction from FL service";
            return;
          }

          Task task = Task(task_id, type, token);
          task_list.push_back(task);
        }

        VLOG(2) << "Received " << task_list.size() << " tasks from FL service";
        std::move(callback).Run(task_list, 60 * 60);
        return;
      } else {
        VLOG(2) << "Reconnecting";
        std::move(callback).Run({}, 10);
      }
    } else {
      VLOG(1) << "Failed to parse GetTasksResponse";
    }
  }

  VLOG(1) << "Failed to get tasks. Response code: " << response_code;
}

void CommunicationAdapter::PostTaskResult(TaskResult result,
                                          PostResultCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kLocalServerResultsEndpoint);
  request->headers.SetHeader("Content-Type", "application/protobuf");
  request->headers.SetHeader("Accept", "application/protobuf");
  request->headers.SetHeader("X-Brave-FL-Federated-Learning", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Posting Task results " << request->method << " " << request->url;

  const std::string& payload = BuildPostTaskResultsPayload(result);

  flower::Result task_result;
  task_result.ParseFromString(payload);

  VLOG(2) << "Payload " << payload;

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(std::move(payload),
                                     "application/protobuf");
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
