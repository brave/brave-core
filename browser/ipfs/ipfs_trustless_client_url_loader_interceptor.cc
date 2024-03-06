/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_trustless_client_url_loader_interceptor.h"
#include <memory>
#include <thread>
#include <utility>
#include "base/task/current_thread.h"
#include "brave/browser/ipfs/ipfs_interrequest_state.h"
#include "brave/browser/ipfs/ipfs_trustless_client_url_loader.h"
#include "brave/browser/ipfs/ipld/block_orchestrator_service_factory.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/ipld/block_orchestrator_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace {

static std::string LogThreadId() {
  std::ostringstream ss;
  ss << std::this_thread::get_id();
  ss << " IsUI:" << base::CurrentUIThread::IsSet();
  return ss.str();
}

bool IsIpfsLink(const GURL& url, PrefService* prefs) {
  return ipfs::IsIPFSScheme(url) || ipfs::HasIPFSPath(url) ||
         ipfs::IsLocalGatewayURL(url) || ipfs::IsDefaultGatewayURL(url, prefs);
}
}  // namespace

namespace ipfs {

// static
std::unique_ptr<content::URLLoaderRequestInterceptor>
IpfsTrustlessClientUrlLoaderInterceptor::MaybeCreateInterceptor() {
  return std::make_unique<IpfsTrustlessClientUrlLoaderInterceptor>();
}

IpfsTrustlessClientUrlLoaderInterceptor::
    IpfsTrustlessClientUrlLoaderInterceptor() = default;
IpfsTrustlessClientUrlLoaderInterceptor::
    ~IpfsTrustlessClientUrlLoaderInterceptor() = default;

void IpfsTrustlessClientUrlLoaderInterceptor::MaybeCreateLoader(
    const network::ResourceRequest& tentative_resource_request,
    content::BrowserContext* browser_context,
    content::URLLoaderRequestInterceptor::LoaderCallback callback) {

  //auto* state = InterRequestState::FromBrowserContext(browser_context);
  // state.set_network_context(network_context_);

  LOG(INFO) << "[IPFS] MaybeCreateLoader url:"
            << tentative_resource_request.url.spec()
            << " thread:" << LogThreadId();

  if (IsIpfsLink(tentative_resource_request.url,
                 user_prefs::UserPrefs::Get(browser_context))) {

  auto orch = std::make_unique<ipld::BlockOrchestratorService>(user_prefs::UserPrefs::Get(browser_context));

  loader_ = std::make_unique<IpfsTrustlessClientIrlLoader>(
            (static_cast<Profile*>(browser_context))->GetURLLoaderFactory(),
            *ipld::BlockOrchestratorServiceFactory::GetServiceForContext(browser_context)
                );

    std::move(callback).Run(base::BindOnce(
        &IpfsTrustlessClientUrlLoaderInterceptor::StartRequest,
        weak_factory_.GetWeakPtr(),
        loader_.get()));
  } else {
    std::move(callback).Run({});
  }
}

// bool IpfsTrustlessClientUrlLoaderInterceptor::MaybeCreateLoaderForResponse(
//     const network::URLLoaderCompletionStatus& status,
//     const network::ResourceRequest& request,
//     network::mojom::URLResponseHeadPtr* response_head,
//     mojo::ScopedDataPipeConsumerHandle* response_body,
//     mojo::PendingRemote<network::mojom::URLLoader>* loader,
//     mojo::PendingReceiver<network::mojom::URLLoaderClient>* client_receiver,
//     blink::ThrottlingURLLoader* url_loader,
//     bool* skip_other_interceptors,
//     bool* will_return_unsafe_redirect) {
//     LOG(INFO) << "[IPFS]
//     IpfsTrustlessClientUrlLoaderInterceptor::MaybeCreateLoaderForResponse
//     URL:" <<   request.url; return
//     content::URLLoaderRequestInterceptor::MaybeCreateLoaderForResponse(
//         status, request, response_head, response_body, loader,
//         client_receiver, url_loader, skip_other_interceptors,
//         will_return_unsafe_redirect);
// }

void IpfsTrustlessClientUrlLoaderInterceptor::StartRequest(
    IpfsTrustlessClientIrlLoader* loader,
    network::ResourceRequest const& resource_request,
    mojo::PendingReceiver<network::mojom::URLLoader> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  loader->StartRequest(resource_request, std::move(receiver),
                       std::move(client));  
}

}  // namespace ipfs
