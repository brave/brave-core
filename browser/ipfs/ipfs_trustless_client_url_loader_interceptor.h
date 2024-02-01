/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_TRUSTLESS_CLIENT_URL_LOADER_INTERCEPTOR_H_
#define BRAVE_BROWSER_IPFS_IPFS_TRUSTLESS_CLIENT_URL_LOADER_INTERCEPTOR_H_

#include <memory>
#include "content/public/browser/url_loader_request_interceptor.h"

namespace network::mojom {
class URLLoaderFactory;
}  // namespace network::mojom


namespace ipfs {

class IpfsTrustlessClientIrlLoader;

class IpfsTrustlessClientUrlLoaderInterceptor
    : public content::URLLoaderRequestInterceptor {
 public:
  static std::unique_ptr<content::URLLoaderRequestInterceptor>
  MaybeCreateInterceptor();

    void MaybeCreateLoader(
      const network::ResourceRequest& tentative_resource_request,
      content::BrowserContext* browser_context,
      content::URLLoaderRequestInterceptor::LoaderCallback callback) override;
    
    // bool MaybeCreateLoaderForResponse(
    //   const network::URLLoaderCompletionStatus& status,
    //   const network::ResourceRequest& request,
    //   network::mojom::URLResponseHeadPtr* response_head,
    //   mojo::ScopedDataPipeConsumerHandle* response_body,
    //   mojo::PendingRemote<network::mojom::URLLoader>* loader,
    //   mojo::PendingReceiver<network::mojom::URLLoaderClient>* client_receiver,
    //   blink::ThrottlingURLLoader* url_loader,
    //   bool* skip_other_interceptors,
    //   bool* will_return_unsafe_redirect) override;

  IpfsTrustlessClientUrlLoaderInterceptor();
  ~IpfsTrustlessClientUrlLoaderInterceptor() override;

  IpfsTrustlessClientUrlLoaderInterceptor(
      const IpfsTrustlessClientUrlLoaderInterceptor&) = delete;
  IpfsTrustlessClientUrlLoaderInterceptor(
      IpfsTrustlessClientUrlLoaderInterceptor&&) = delete;
  IpfsTrustlessClientUrlLoaderInterceptor& operator=(
      const IpfsTrustlessClientUrlLoaderInterceptor&) = delete;
  IpfsTrustlessClientUrlLoaderInterceptor& operator=(
      IpfsTrustlessClientUrlLoaderInterceptor&&) = delete;

private:
     void StartRequest(
       std::unique_ptr<IpfsTrustlessClientIrlLoader> loader,
       network::ResourceRequest const& resource_request,
       mojo::PendingReceiver<network::mojom::URLLoader> receiver,
       mojo::PendingRemote<network::mojom::URLLoaderClient> client);
    
    raw_ptr<network::mojom::URLLoaderFactory> loader_factory_;
    base::WeakPtrFactory<IpfsTrustlessClientUrlLoaderInterceptor> weak_factory_{this};
};

}
#endif  // BRAVE_BROWSER_IPFS_IPFS_TRUSTLESS_CLIENT_URL_LOADER_INTERCEPTOR_H_