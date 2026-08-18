/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_SERVICES_NETWORK_SIMPLE_URL_LOADER_SERVICE_H_
#define BRAVE_SERVICES_NETWORK_SIMPLE_URL_LOADER_SERVICE_H_

#include <memory>
#include <set>

#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/services/network/public/mojom/simple_url_loader.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave::network {

// Serves the brave.network.mojom.SimpleUrlLoader interface on top of
// network::SimpleURLLoader, so that non-C++ callers (currently Rust) can issue
// HTTP requests through Chromium's network stack without reimplementing
// URLLoaderClient, data pipe draining, redirect handling and retry policy.
//
// The URLLoaderFactory and the traffic annotation are both fixed at
// construction. The annotation deliberately cannot come from the caller:
// network traffic annotations must be compile-time constants so that
// traffic_annotation_auditor can find them. Each feature that wants to make
// requests from Rust should therefore construct its own instance with its own
// annotation, exactly as it would define one at a C++ call site today.
//
// Lives on the sequence it was constructed on. In-flight requests are
// cancelled on destruction.
class SimpleUrlLoaderService : public mojom::SimpleUrlLoader {
 public:
  // `json_task_runner` runs JSON sanitization (see
  // DownloadRequest.sanitize_json_response), which is kept off this sequence
  // because parsing untrusted input can be slow. Defaults to a thread pool
  // sequence; tests may inject their own.
  SimpleUrlLoaderService(
      scoped_refptr<::network::SharedURLLoaderFactory> url_loader_factory,
      net::NetworkTrafficAnnotationTag traffic_annotation,
      scoped_refptr<base::SequencedTaskRunner> json_task_runner = nullptr);

  SimpleUrlLoaderService(const SimpleUrlLoaderService&) = delete;
  SimpleUrlLoaderService& operator=(const SimpleUrlLoaderService&) = delete;

  ~SimpleUrlLoaderService() override;

  void Bind(mojo::PendingReceiver<mojom::SimpleUrlLoader> receiver);

  // mojom::SimpleUrlLoader:
  void Download(mojom::DownloadRequestPtr request,
                mojo::PendingReceiver<mojom::DownloadHandle> cancellation,
                DownloadCallback callback) override;

 private:
  // One in-flight request. Owns its network::SimpleURLLoader, so destroying it
  // cancels the load.
  class InFlightRequest;

  // Destroys `request`, which must be owned by `requests_`.
  void Finish(InFlightRequest* request);

  scoped_refptr<::network::SharedURLLoaderFactory> url_loader_factory_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;
  scoped_refptr<base::SequencedTaskRunner> json_task_runner_;

  std::set<std::unique_ptr<InFlightRequest>, base::UniquePtrComparator>
      requests_;

  mojo::ReceiverSet<mojom::SimpleUrlLoader> receivers_;
};

}  // namespace brave::network

#endif  // BRAVE_SERVICES_NETWORK_SIMPLE_URL_LOADER_SERVICE_H_
