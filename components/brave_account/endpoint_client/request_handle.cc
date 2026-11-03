/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/request_handle.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_account::endpoint_client {

RequestHandle::RequestHandle() = default;

RequestHandle::RequestHandle(RequestHandle&&) = default;

RequestHandle& RequestHandle::operator=(RequestHandle&&) = default;

RequestHandle::~RequestHandle() = default;

RequestHandle::RequestHandle(
    std::unique_ptr<network::SimpleURLLoader> loader,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : loader_(loader.release(), Deleter(std::move(task_runner))) {}

RequestHandle::Deleter::Deleter(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

RequestHandle::Deleter::Deleter(Deleter&&) = default;

RequestHandle::Deleter& RequestHandle::Deleter::operator=(Deleter&&) = default;

RequestHandle::Deleter::~Deleter() = default;

void RequestHandle::Deleter::operator()(
    network::SimpleURLLoader* loader) const {
  CHECK(CHECK_DEREF(task_runner_).RunsTasksInCurrentSequence());
  delete loader;
}

}  // namespace brave_account::endpoint_client
