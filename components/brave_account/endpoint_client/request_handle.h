/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_HANDLE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_HANDLE_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_account::endpoint_client {

template <IsEndpoint>
class Client;

namespace detail {

// Sequence-aware deleter (marshals destruction to |task_runner_|).
class RequestHandleDeleter {
 public:
  RequestHandleDeleter(RequestHandleDeleter&&);
  RequestHandleDeleter& operator=(RequestHandleDeleter&&);

  ~RequestHandleDeleter();

  void operator()(void* ptr) const;

 private:
  template <IsEndpoint>
  friend class ::brave_account::endpoint_client::Client;

  explicit RequestHandleDeleter(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace detail

// RequestHandle is intentionally opaque: it hides the managed SimpleURLLoader
// to prevent direct API access. Callers should only be able to hold, move, or
// reset the handle to cancel a request - hence the type erasure.
using RequestHandle = std::unique_ptr<void, detail::RequestHandleDeleter>;

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_HANDLE_H_
