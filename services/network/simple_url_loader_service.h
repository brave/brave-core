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
#include "base/memory/weak_ptr.h"
#include "brave/services/network/public/mojom/simple_url_loader.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {
class SimpleURLLoader;
}  // namespace network

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
// Lives on the sequence it was constructed on. In-flight loaders are cancelled
// on destruction and their callbacks are dropped, which closes the response
// callback and surfaces as a disconnect on the caller's side.
class SimpleUrlLoaderService : public mojom::SimpleUrlLoader {
 public:
  SimpleUrlLoaderService(
      scoped_refptr<::network::SharedURLLoaderFactory> url_loader_factory,
      net::NetworkTrafficAnnotationTag traffic_annotation);

  SimpleUrlLoaderService(const SimpleUrlLoaderService&) = delete;
  SimpleUrlLoaderService& operator=(const SimpleUrlLoaderService&) = delete;

  ~SimpleUrlLoaderService() override;

  void Bind(mojo::PendingReceiver<mojom::SimpleUrlLoader> receiver);

  // mojom::SimpleUrlLoader:
  void Download(mojom::DownloadRequestPtr request,
                DownloadCallback callback) override;

 private:
  void OnDownloadComplete(::network::SimpleURLLoader* loader,
                          DownloadCallback callback,
                          std::optional<std::string> body);

  scoped_refptr<::network::SharedURLLoaderFactory> url_loader_factory_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  // Owns in-flight loaders. Erased when their callback runs.
  std::set<std::unique_ptr<::network::SimpleURLLoader>,
           base::UniquePtrComparator>
      loaders_;

  mojo::ReceiverSet<mojom::SimpleUrlLoader> receivers_;

  base::WeakPtrFactory<SimpleUrlLoaderService> weak_factory_{this};
};

}  // namespace brave::network

#endif  // BRAVE_SERVICES_NETWORK_SIMPLE_URL_LOADER_SERVICE_H_
