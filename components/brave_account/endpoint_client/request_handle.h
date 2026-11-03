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

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace brave_account::endpoint_client {

// RequestHandle is intentionally opaque: it hides the managed SimpleURLLoader
// to prevent direct API access. Callers can only hold, move, or reset the
// handle to cancel a request, and only Client<> can create a non-empty one.
class RequestHandle {
 public:
  RequestHandle();

  RequestHandle(RequestHandle&&);
  RequestHandle& operator=(RequestHandle&&);

  ~RequestHandle();

  void reset() { loader_.reset(); }

 private:
  template <IsEndpoint>
  friend class Client;

  // Deletes the RequestHandle's SimpleURLLoader inline, so cancellation is
  // synchronous. CHECKs that this happens on the sequence that issued the
  // request, the only one the loader may be touched from.
  class Deleter {
   public:
    explicit Deleter(
        scoped_refptr<base::SequencedTaskRunner> task_runner = nullptr);

    Deleter(Deleter&&);
    Deleter& operator=(Deleter&&);

    ~Deleter();

    void operator()(network::SimpleURLLoader* loader) const;

   private:
    scoped_refptr<base::SequencedTaskRunner> task_runner_;
  };

  RequestHandle(std::unique_ptr<network::SimpleURLLoader> loader,
                scoped_refptr<base::SequencedTaskRunner> task_runner);

  std::unique_ptr<network::SimpleURLLoader, Deleter> loader_;
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_HANDLE_H_
