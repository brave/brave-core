/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_COMMUNICATION_ADAPTER_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_COMMUNICATION_ADAPTER_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/task/typing.h"
#include "net/base/backoff_entry.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_federated {

class TaskResultResponse;

// This is meant to be an adapter class with a set interface, but since
// we only have one possible adaptations (flower) this is also the flower
// specific implementation
class CommunicationAdapter {
 public:
  explicit CommunicationAdapter(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const net::BackoffEntry::Policy reconnect_policy,
      const net::BackoffEntry::Policy request_task_policy);
  ~CommunicationAdapter();

  using UploadResultCallback = base::OnceCallback<void(TaskResultResponse)>;
  using GetTaskCallback = base::OnceCallback<void(TaskList, base::TimeDelta)>;

  void GetTasks(GetTaskCallback callback);
  void OnGetTasks(GetTaskCallback callback,
                  const std::unique_ptr<std::string> response_body);
  void UploadTaskResult(const TaskResult& result,
                        UploadResultCallback callback);
  void OnUploadTaskResult(UploadResultCallback callback,
                          const std::unique_ptr<std::string> response_body);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  std::unique_ptr<const net::BackoffEntry::Policy> reconnect_policy_;
  std::unique_ptr<net::BackoffEntry> reconnect_backoff_entry_;
  std::unique_ptr<const net::BackoffEntry::Policy> request_task_policy_;
  std::unique_ptr<net::BackoffEntry> request_task_backoff_entry_;

  base::WeakPtrFactory<CommunicationAdapter> weak_factory_{this};
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_COMMUNICATION_ADAPTER_H_
