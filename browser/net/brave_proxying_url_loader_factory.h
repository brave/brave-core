/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_PROXYING_URL_LOADER_FACTORY_H_
#define BRAVE_BROWSER_NET_BRAVE_PROXYING_URL_LOADER_FACTORY_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/browser/net/resource_context_data.h"
#include "brave/browser/net/url_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "net/base/completion_once_callback.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom-forward.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
}  // namespace content

// Cargoculted from WebRequestProxyingURLLoaderFactory and
// signin::ProxyingURLLoaderFactory
class BraveProxyingURLLoaderFactory
    : public network::mojom::URLLoaderFactory {
 public:
  using DisconnectCallback =
      base::OnceCallback<void(BraveProxyingURLLoaderFactory*)>;

  class InProgressRequest : public network::mojom::URLLoader,
                            public network::mojom::URLLoaderClient {
   public:
    InProgressRequest(
        BraveProxyingURLLoaderFactory* factory,
        uint64_t request_id,
        int32_t network_service_request_id,
        int render_process_id,
        int frame_tree_node_id,
        uint32_t options,
        const network::ResourceRequest& request,
        content::BrowserContext* browser_context,
        const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
        mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
        mojo::PendingRemote<network::mojom::URLLoaderClient> client);
    InProgressRequest(const InProgressRequest&) = delete;
    InProgressRequest& operator=(const InProgressRequest&) = delete;
    ~InProgressRequest() override;

    void Restart();

    // network::mojom::URLLoader:
    void FollowRedirect(
        const std::vector<std::string>& removed_headers,
        const net::HttpRequestHeaders& modified_headers,
        const net::HttpRequestHeaders& modified_cors_exempt_headers,
        const absl::optional<GURL>& new_url) override;
    void SetPriority(net::RequestPriority priority,
                     int32_t intra_priority_value) override;
    void PauseReadingBodyFromNet() override;
    void ResumeReadingBodyFromNet() override;

    // network::mojom::URLLoaderClient:
    void OnReceiveEarlyHints(
        network::mojom::EarlyHintsPtr early_hints) override;
    void OnReceiveResponse(
        network::mojom::URLResponseHeadPtr response_head) override;
    void OnReceiveRedirect(
        const net::RedirectInfo& redirect_info,
        network::mojom::URLResponseHeadPtr response_head) override;
    void OnUploadProgress(int64_t current_position,
                          int64_t total_size,
                          OnUploadProgressCallback callback) override;
    void OnReceiveCachedMetadata(mojo_base::BigBuffer data) override;
    void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
    void OnStartLoadingResponseBody(
        mojo::ScopedDataPipeConsumerHandle body) override;
    void OnComplete(const network::URLLoaderCompletionStatus& status) override;

   private:
    // These two methods combined form the implementation of Restart().
    void UpdateRequestInfo();
    void RestartInternal();

    void ContinueToBeforeSendHeaders(int error_code);
    void ContinueToSendHeaders(int error_code);
    void ContinueToStartRequest(int error_code);
    void ContinueToResponseStarted(int error_code);
    void ContinueToBeforeRedirect(const net::RedirectInfo& redirect_info,
                                  int error_code);
    void HandleResponseOrRedirectHeaders(
        net::CompletionOnceCallback continuation);
    void OnRequestError(const network::URLLoaderCompletionStatus& status);
    void HandleBeforeRequestRedirect();

    base::TimeTicks start_time_;

    // TODO(iefremov): Get rid of shared_ptr, we should clearly own the pointer.
    std::shared_ptr<brave::BraveRequestInfo> ctx_;
    BraveProxyingURLLoaderFactory* const factory_;
    network::ResourceRequest request_;
    const uint64_t request_id_;
    const int32_t network_service_request_id_;

    const int render_process_id_;
    const int frame_tree_node_id_;
    const uint32_t options_;

    content::BrowserContext* browser_context_;
    const net::MutableNetworkTrafficAnnotationTag traffic_annotation_;

    // This is our proxy's receiver that will talk to the original client. It
    // will take over the passed in PendingReceiver.
    mojo::Receiver<network::mojom::URLLoader> proxied_loader_receiver_;
    // This is the original client.
    mojo::Remote<network::mojom::URLLoaderClient> target_client_;

    // This is our proxy's client that will talk to originally targeted loader.
    mojo::Receiver<network::mojom::URLLoaderClient> proxied_client_receiver_;
    // This is the original receiver the original client meant to talk to.
    mojo::Remote<network::mojom::URLLoader> target_loader_;

    // NOTE: This is state which ExtensionWebRequestEventRouter needs to have
    // persisted across some phases of this request -- namely between
    // |OnHeadersReceived()| and request completion or restart. Pointers to
    // these fields are stored in a |BlockedRequest| (created and owned by
    // ExtensionWebRequestEventRouter) through much of the request's lifetime.
    // That code supports both Network Service and non-Network Service behavior,
    // which is why this weirdness exists here.
    network::mojom::URLResponseHeadPtr current_response_;
    scoped_refptr<net::HttpResponseHeaders> override_headers_;
    GURL redirect_url_;

    bool request_completed_ = false;

    // This stores the parameters to FollowRedirect that came from
    // the client. That way we can combine it with any other changes that
    // extensions made to headers in their callbacks.
    struct FollowRedirectParams {
      FollowRedirectParams();
      FollowRedirectParams(const FollowRedirectParams&) = delete;
      FollowRedirectParams& operator=(const FollowRedirectParams&) = delete;
      ~FollowRedirectParams();
      std::vector<std::string> removed_headers;
      net::HttpRequestHeaders modified_headers;
      net::HttpRequestHeaders modified_cors_exempt_headers;
      absl::optional<GURL> new_url;
    };
    std::unique_ptr<FollowRedirectParams> pending_follow_redirect_params_;

    base::WeakPtrFactory<InProgressRequest> weak_factory_;
  };

  // Constructor public for testing purposes. New instances should be created
  // by calling MaybeProxyRequest().
  BraveProxyingURLLoaderFactory(
      BraveRequestHandler* request_handler,
      content::BrowserContext* browser_context,
      int render_process_id,
      int frame_tree_node_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      network::mojom::URLLoaderFactoryPtrInfo target_factory,
      scoped_refptr<RequestIDGenerator> request_id_generator,
      DisconnectCallback on_disconnect);

  BraveProxyingURLLoaderFactory(const BraveProxyingURLLoaderFactory&) = delete;
  BraveProxyingURLLoaderFactory& operator=(
      const BraveProxyingURLLoaderFactory&) = delete;

  ~BraveProxyingURLLoaderFactory() override;

  static bool MaybeProxyRequest(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* render_frame_host,
      int render_process_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>*
          factory_receiver);

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;
  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory>
                 loader_receiver) override;

 private:
  friend class base::DeleteHelper<BraveProxyingURLLoaderFactory>;
  friend class base::RefCountedDeleteOnSequence<BraveProxyingURLLoaderFactory>;

  void OnTargetFactoryError();
  void OnProxyBindingError();
  void RemoveRequest(InProgressRequest* request);

  void MaybeRemoveProxy();

  BraveRequestHandler* const request_handler_;
  content::BrowserContext* browser_context_;
  const int render_process_id_;
  const int frame_tree_node_id_;

  mojo::ReceiverSet<network::mojom::URLLoaderFactory> proxy_receivers_;
  network::mojom::URLLoaderFactoryPtr target_factory_;

  std::set<std::unique_ptr<InProgressRequest>, base::UniquePtrComparator>
      requests_;

  scoped_refptr<RequestIDGenerator> request_id_generator_;

  DisconnectCallback disconnect_callback_;

  base::WeakPtrFactory<BraveProxyingURLLoaderFactory> weak_factory_;
};

#endif  // BRAVE_BROWSER_NET_BRAVE_PROXYING_URL_LOADER_FACTORY_H_
