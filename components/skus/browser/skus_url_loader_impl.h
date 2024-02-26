// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_URL_LOADER_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_URL_LOADER_IMPL_H_

#include <memory>
#include <string>

#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/rust/chromium_crates_io/vendor/cxx-1.0.117/include/cxx.h"

namespace skus {

struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;

class SkusUrlLoaderImpl : public SkusUrlLoader {
 public:
  explicit SkusUrlLoaderImpl(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusUrlLoaderImpl() override;

  using FetchResponseCallback = rust::cxxbridge1::Fn<void(
      rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
      skus::HttpResponse)>;
  using FetchResponseCallbackForTesting =
      base::OnceCallback<void(const skus::HttpResponse&)>;
  void BeginFetch(
      const skus::HttpRequest& req,
      FetchResponseCallback callback,
      rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx) override;

  void Request(const std::string& method,
               const GURL& url,
               const std::string& payload,
               const std::string& payload_content_type,
               api_request_helper::APIRequestHelper::ResultCallback callback,
               const base::flat_map<std::string, std::string>& headers,
               const api_request_helper::APIRequestOptions& request_options);
  void SetFetchCompleteCallbackForTesting(
      FetchResponseCallbackForTesting callback);

 private:
  friend class SkusUrlLoaderImplUnitTest;

  void OnFetchComplete(FetchResponseCallback callback,
                       rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx,
                       api_request_helper::APIRequestResult api_request_result);

  FetchResponseCallbackForTesting fetch_complete_callback_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_URL_LOADER_IMPL_H_
