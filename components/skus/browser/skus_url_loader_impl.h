// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_URL_LOADER_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_URL_LOADER_IMPL_H_

#include <memory>
#include <string>

#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/rust/cxx/v1/crate/include/cxx.h"

namespace skus {

struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;

class SkusUrlLoaderImpl : public SkusUrlLoader {
 public:
  explicit SkusUrlLoaderImpl(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusUrlLoaderImpl() override;

  void BeginFetch(
      const skus::HttpRequest& req,
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
               skus::HttpResponse)> callback,
      rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx) override;

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;

  const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag();

  void OnFetchComplete(rust::cxxbridge1::Fn<void(
                           rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                           skus::HttpResponse)> callback,
                       rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx,
                       std::unique_ptr<std::string> response_body);
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_URL_LOADER_IMPL_H_
