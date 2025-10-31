/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/request_handle.h"

#include <utility>

#include "base/check.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_account::endpoint_client::detail {

RequestHandleDeleter::RequestHandleDeleter(RequestHandleDeleter&&) = default;

RequestHandleDeleter& RequestHandleDeleter::operator=(RequestHandleDeleter&&) =
    default;

RequestHandleDeleter::~RequestHandleDeleter() = default;

void RequestHandleDeleter::operator()(void* ptr) const {
  if (ptr) {
    task_runner_->DeleteSoon(FROM_HERE,
                             static_cast<network::SimpleURLLoader*>(ptr));
  }
}

RequestHandleDeleter::RequestHandleDeleter(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {
  CHECK(task_runner_);
}

}  // namespace brave_account::endpoint_client::detail
