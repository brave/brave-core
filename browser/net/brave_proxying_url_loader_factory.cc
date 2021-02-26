/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/components/brave_shields/browser/adblock_stub_response.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/url_utils.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/completion_repeating_callback.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_util.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/redirect_util.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/features.h"
#include "url/origin.h"

namespace {

// Helper struct for crafting responses.
struct WriteData {
  base::WeakPtr<network::mojom::URLLoaderClient> client;
  std::string data;
  std::unique_ptr<mojo::DataPipeProducer> producer;
};

void OnWrite(std::unique_ptr<WriteData> write_data, MojoResult result) {
  if (result != MOJO_RESULT_OK || !write_data->client) {
    return;
  }

  network::URLLoaderCompletionStatus status(net::OK);
  status.encoded_data_length = write_data->data.size();
  status.encoded_body_length = write_data->data.size();
  status.decoded_body_length = write_data->data.size();
  write_data->client->OnComplete(status);
}

// Creates simulated net::RedirectInfo when an extension redirects a request,
// behaving like a redirect response was actually returned by the remote server.
net::RedirectInfo CreateRedirectInfo(
    const network::ResourceRequest& original_request,
    const GURL& new_url,
    int response_code,
    const base::Optional<std::string>& referrer_policy_header) {
  // Workaround for a bug in Chromium (crbug.com/1097681).
  // download_utils.cc do not set update_first_party_url_on_redirect to true
  // for new ResourceRequests, but we can mitigate it by looking at
  // |is_main_frame| which is true for navigations and downloads.
  const bool update_first_party_url_on_redirect =
      (original_request.update_first_party_url_on_redirect ||
       original_request.is_main_frame);
  return net::RedirectInfo::ComputeRedirectInfo(
      original_request.method, original_request.url,
      original_request.site_for_cookies,
      update_first_party_url_on_redirect
          ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
          : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL,
      original_request.referrer_policy, original_request.referrer.spec(),
      response_code, new_url, referrer_policy_header,
      false /* insecure_scheme_was_upgraded */, false /* copy_fragment */,
      false /* is_signed_exchange_fallback_redirect */);
}

}  // namespace

BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    FollowRedirectParams() = default;
BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    ~FollowRedirectParams() = default;

BraveProxyingURLLoaderFactory::InProgressRequest::InProgressRequest(
    BraveProxyingURLLoaderFactory* factory,
    uint64_t request_id,
    int32_t network_service_request_id,
    int32_t routing_id,
    int render_process_id,
    int frame_tree_node_id,
    uint32_t options,
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client)
    : factory_(factory),
      request_(request),
      request_id_(request_id),
      network_service_request_id_(network_service_request_id),
      render_process_id_(render_process_id),
      frame_tree_node_id_(frame_tree_node_id),
      routing_id_(routing_id),
      options_(options),
      browser_context_(browser_context),
      traffic_annotation_(traffic_annotation),
      proxied_loader_receiver_(this, std::move(loader_receiver)),
      target_client_(std::move(client)),
      proxied_client_receiver_(this),
      weak_factory_(this) {
  // If there is a client error, clean up the request.
  target_client_.set_disconnect_handler(base::BindOnce(
      &BraveProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
      weak_factory_.GetWeakPtr(),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED)));
}

BraveProxyingURLLoaderFactory::InProgressRequest::~InProgressRequest() {
  if (ctx_) {
    factory_->request_handler_->OnURLRequestDestroyed(ctx_);
  }
}

void BraveProxyingURLLoaderFactory::InProgressRequest::Restart() {
  UpdateRequestInfo();
  RestartInternal();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::UpdateRequestInfo() {
  // TODO(iefremov): Update |ctx_| here and get rid of multiple spots where
  // it is refilled.
}

void BraveProxyingURLLoaderFactory::InProgressRequest::RestartInternal() {
  request_completed_ = false;
  start_time_ = base::TimeTicks::Now();

  base::RepeatingCallback<void(int)> continuation =
      base::BindRepeating(&InProgressRequest::ContinueToBeforeSendHeaders,
                          weak_factory_.GetWeakPtr());
  redirect_url_ = GURL();
  ctx_ = brave::BraveRequestInfo::MakeCTX(request_, render_process_id_,
                                          frame_tree_node_id_, request_id_,
                                          browser_context_, ctx_);
  int result = factory_->request_handler_->OnBeforeURLRequest(
      ctx_, continuation, &redirect_url_);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // The request was cancelled synchronously. Dispatch an error notification
    // and terminate the request.
    network::URLLoaderCompletionStatus status(result);
    OnRequestError(status);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // One or more listeners is blocking, so the request must be paused until
    // they respond. |continuation| above will be invoked asynchronously to
    // continue or cancel the request.
    //
    // We pause the binding here to prevent further client message processing.
    if (proxied_client_receiver_.is_bound())
      proxied_client_receiver_.Pause();

    return;
  }
  DCHECK_EQ(net::OK, result);

  continuation.Run(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const base::Optional<GURL>& new_url) {
  if (new_url)
    request_.url = new_url.value();

  for (const std::string& header : removed_headers)
    request_.headers.RemoveHeader(header);
  request_.headers.MergeFrom(modified_headers);

  UpdateRequestInfo();

  if (target_loader_.is_bound()) {
    auto params = std::make_unique<FollowRedirectParams>();
    params->removed_headers = removed_headers;
    params->modified_headers = modified_headers;
    params->modified_cors_exempt_headers = modified_cors_exempt_headers;
    params->new_url = new_url;
    pending_follow_redirect_params_ = std::move(params);
  }

  RestartInternal();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound())
    target_loader_->SetPriority(priority, intra_priority_value);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    PauseReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->PauseReadingBodyFromNet();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->ResumeReadingBodyFromNet();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr head) {
  current_response_ = std::move(head);
  ctx_->internal_redirect = false;
  HandleResponseOrRedirectHeaders(
      base::BindRepeating(&InProgressRequest::ContinueToResponseStarted,
                          weak_factory_.GetWeakPtr()));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  current_response_ = std::move(head);
  DCHECK(ctx_);
  ctx_->internal_redirect = false;
  HandleResponseOrRedirectHeaders(
      base::BindRepeating(&InProgressRequest::ContinueToBeforeRedirect,
                          weak_factory_.GetWeakPtr(), redirect_info));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveCachedMetadata(
    mojo_base::BigBuffer data) {
  target_client_->OnReceiveCachedMetadata(std::move(data));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    OnStartLoadingResponseBody(mojo::ScopedDataPipeConsumerHandle body) {
  target_client_->OnStartLoadingResponseBody(std::move(body));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  UMA_HISTOGRAM_TIMES("Brave.ProxyingURLLoader.TotalRequestTime",
                      base::TimeTicks::Now() - start_time_);
  if (status.error_code != net::OK) {
    OnRequestError(status);
    return;
  }
  target_client_->OnComplete(status);

  // Deletes |this|.
  factory_->RemoveRequest(this);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    HandleBeforeRequestRedirect() {
  // The listener requested a redirect. Close the connection with the current
  // URLLoader and inform the URLLoaderClient redirect was generated.
  // To load |redirect_url_|, a new URLLoader will be recreated
  // after receiving FollowRedirect().

  // Forgetting to close the connection with the current URLLoader caused
  // bugs. The latter doesn't know anything about the redirect. Continuing
  // the load with it gives unexpected results. See
  // https://crbug.com/882661#c72.
  proxied_client_receiver_.reset();
  target_loader_.reset();

  constexpr int kInternalRedirectStatusCode = 307;

  net::RedirectInfo redirect_info =
      CreateRedirectInfo(request_, redirect_url_, kInternalRedirectStatusCode,
                         base::nullopt /* referrer_policy_header */);

  network::mojom::URLResponseHeadPtr head =
      network::mojom::URLResponseHead::New();
  std::string headers = base::StringPrintf(
      "HTTP/1.1 %i Internal Redirect\n"
      "Location: %s\n"
      "Non-Authoritative-Reason: WebRequest API\n\n",
      kInternalRedirectStatusCode, redirect_url_.spec().c_str());

  // Cross-origin requests need to modify the Origin header to 'null'. Since
  // CorsURLLoader sets |request_initiator| to the Origin request header in
  // NetworkService, we need to modify |request_initiator| here to craft the
  // Origin header indirectly.
  // Following checks implement the step 10 of "4.4. HTTP-redirect fetch",
  // https://fetch.spec.whatwg.org/#http-redirect-fetch
  if (request_.request_initiator &&
      (!url::Origin::Create(redirect_url_)
            .IsSameOriginWith(url::Origin::Create(request_.url)) &&
       !request_.request_initiator->IsSameOriginWith(
           url::Origin::Create(request_.url)))) {
    // Reset the initiator to pretend tainted origin flag of the spec is set.
    request_.request_initiator = url::Origin();
  }
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers));
  head->encoded_data_length = 0;

  current_response_ = std::move(head);
  ctx_->internal_redirect = true;
  ContinueToBeforeRedirect(redirect_info, net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToBeforeSendHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (!redirect_url_.is_empty()) {
    HandleBeforeRequestRedirect();
    return;
  }

  DCHECK(ctx_);
  if (ctx_->new_referrer.has_value()) {
    request_.referrer = ctx_->new_referrer.value();
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  // TODO(iefremov): Shorten
  if (ctx_->blocked_by != brave::kNotBlocked) {
    if (!ctx_->ShouldMockRequest()) {
      OnRequestError(
          network::URLLoaderCompletionStatus(net::ERR_BLOCKED_BY_CLIENT));
      return;
    }

    auto response = network::mojom::URLResponseHead::New();
    std::string response_data;
    brave_shields::MakeStubResponse(ctx_->mock_data_url, request_, &response,
                                    &response_data);

    target_client_->OnReceiveResponse(std::move(response));

    // Create a data pipe for transmitting the response.
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
      OnRequestError(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }

    // Craft the response.
    target_client_->OnStartLoadingResponseBody(std::move(consumer));

    auto write_data = std::make_unique<WriteData>();
    write_data->client = weak_factory_.GetWeakPtr();
    write_data->data = response_data;
    write_data->producer =
        std::make_unique<mojo::DataPipeProducer>(std::move(producer));

    base::StringPiece string_piece(write_data->data);
    write_data->producer->Write(
        std::make_unique<mojo::StringDataSource>(
            string_piece, mojo::StringDataSource::AsyncWritingMode::
                              STRING_STAYS_VALID_UNTIL_COMPLETION),
        base::BindOnce(OnWrite, std::move(write_data)));
    return;
  }

  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    auto continuation = base::BindRepeating(
        &InProgressRequest::ContinueToSendHeaders, weak_factory_.GetWeakPtr());

    ctx_ = brave::BraveRequestInfo::MakeCTX(request_, render_process_id_,
                                            frame_tree_node_id_, request_id_,
                                            browser_context_, ctx_);
    int result = factory_->request_handler_->OnBeforeStartTransaction(
        ctx_, continuation, &request_.headers);

    if (result == net::ERR_BLOCKED_BY_CLIENT) {
      // The request was cancelled synchronously. Dispatch an error notification
      // and terminate the request.
      OnRequestError(network::URLLoaderCompletionStatus(result));
      return;
    }

    if (result == net::ERR_IO_PENDING) {
      // One or more listeners is blocking, so the request must be paused until
      // they respond. |continuation| above will be invoked asynchronously to
      // continue or cancel the request.
      //
      // We pause the binding here to prevent further client message processing.
      if (proxied_client_receiver_.is_bound())
        proxied_client_receiver_.Pause();
      return;
    }
    DCHECK_EQ(net::OK, result);
  }

  ContinueToSendHeaders(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::ContinueToStartRequest(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  if (!target_loader_.is_bound() && factory_->target_factory_.is_bound()) {
    // Nothing has cancelled us up to this point, so it's now OK to
    // initiate the real network request.
    uint32_t options = options_;
    factory_->target_factory_->CreateLoaderAndStart(
        target_loader_.BindNewPipeAndPassReceiver(), routing_id_,
        network_service_request_id_, options, request_,
        proxied_client_receiver_.BindNewPipeAndPassRemote(),
        traffic_annotation_);
  }

  // From here the lifecycle of this request is driven by subsequent events on
  // either |proxied_loader_receiver_|, |proxied_client_receiver_|.
}

void BraveProxyingURLLoaderFactory::InProgressRequest::ContinueToSendHeaders(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }
  const std::set<std::string>& removed_headers = ctx_->removed_headers;
  const std::set<std::string>& set_headers = ctx_->set_headers;

  if (pending_follow_redirect_params_) {
    pending_follow_redirect_params_->removed_headers.insert(
        pending_follow_redirect_params_->removed_headers.end(),
        removed_headers.begin(), removed_headers.end());

    for (auto& set_header : set_headers) {
      std::string header_value;
      if (request_.headers.GetHeader(set_header, &header_value)) {
        pending_follow_redirect_params_->modified_headers.SetHeader(
            set_header, header_value);
      } else {
        NOTREACHED();
      }
    }

    if (target_loader_.is_bound()) {
      target_loader_->FollowRedirect(
          pending_follow_redirect_params_->removed_headers,
          pending_follow_redirect_params_->modified_headers,
          pending_follow_redirect_params_->modified_cors_exempt_headers,
          pending_follow_redirect_params_->new_url);
    }

    pending_follow_redirect_params_.reset();
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();
  ContinueToStartRequest(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToResponseStarted(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (override_headers_)
    current_response_->headers = override_headers_;

  std::string redirect_location;
  if (override_headers_ && override_headers_->IsRedirect(&redirect_location)) {
    // The response headers may have been overridden by an |onHeadersReceived|
    // handler and may have been changed to a redirect. We handle that here
    // instead of acting like regular request completion.
    //
    // Note that we can't actually change how the Network Service handles the
    // original request at this point, so our "redirect" is really just
    // generating an artificial |onBeforeRedirect| event and starting a new
    // request to the Network Service. Our client shouldn't know the difference.
    GURL new_url(redirect_location);

    net::RedirectInfo redirect_info = CreateRedirectInfo(
         request_, new_url, override_headers_->response_code(),
         net::RedirectUtil::GetReferrerPolicyHeader(override_headers_.get()));

    // These will get re-bound if a new request is initiated by
    // |FollowRedirect()|.
    proxied_client_receiver_.reset();
    target_loader_.reset();

    ctx_->internal_redirect = true;
    ContinueToBeforeRedirect(redirect_info, net::OK);
    return;
  }

  proxied_client_receiver_.Resume();
  target_client_->OnReceiveResponse(std::move(current_response_));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::ContinueToBeforeRedirect(
    const net::RedirectInfo& redirect_info,
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  if (ctx_->internal_redirect) {
    ctx_->redirect_source = GURL();
  } else {
    ctx_->redirect_source = request_.url;
  }
  target_client_->OnReceiveRedirect(redirect_info,
                                    std::move(current_response_));
  request_.url = redirect_info.new_url;
  request_.method = redirect_info.new_method;
  request_.site_for_cookies = redirect_info.new_site_for_cookies;
  request_.referrer = GURL(redirect_info.new_referrer);
  request_.referrer_policy = redirect_info.new_referrer_policy;

  if (request_.trusted_params) {
    request_.trusted_params->isolation_info =
        request_.trusted_params->isolation_info.CreateForRedirect(
            url::Origin::Create(redirect_info.new_url));
  }

  // The request method can be changed to "GET". In this case we need to
  // reset the request body manually.
  if (request_.method == net::HttpRequestHeaders::kGetMethod)
    request_.request_body = nullptr;

  request_completed_ = true;
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    HandleResponseOrRedirectHeaders(net::CompletionOnceCallback continuation) {
  override_headers_ = nullptr;
  redirect_url_ = GURL();

  net::CompletionRepeatingCallback copyable_callback =
      base::AdaptCallbackForRepeating(std::move(continuation));
  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    ctx_ = brave::BraveRequestInfo::MakeCTX(request_, render_process_id_,
                                            frame_tree_node_id_, request_id_,
                                            browser_context_, ctx_);
    int result = factory_->request_handler_->OnHeadersReceived(
        ctx_, copyable_callback, current_response_->headers.get(),
        &override_headers_, &redirect_url_);

    if (result == net::ERR_BLOCKED_BY_CLIENT) {
      OnRequestError(network::URLLoaderCompletionStatus(result));
      return;
    }

    if (result == net::ERR_IO_PENDING) {
      // One or more listeners is blocking, so the request must be paused until
      // they respond. |continuation| above will be invoked asynchronously to
      // continue or cancel the request.
      //
      // We pause the binding here to prevent further client message processing.
      proxied_client_receiver_.Pause();
      return;
    }

    DCHECK_EQ(net::OK, result);
  }

  copyable_callback.Run(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnRequestError(
    const network::URLLoaderCompletionStatus& status) {
  if (!request_completed_) {
    target_client_->OnComplete(status);
  }

  // Deletes |this|.
  factory_->RemoveRequest(this);
}

BraveProxyingURLLoaderFactory::BraveProxyingURLLoaderFactory(
    BraveRequestHandler* request_handler,
    content::BrowserContext* browser_context,
    int render_process_id,
    int frame_tree_node_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    network::mojom::URLLoaderFactoryPtrInfo target_factory,
    scoped_refptr<RequestIDGenerator> request_id_generator,
    DisconnectCallback on_disconnect)
    : request_handler_(request_handler),
      browser_context_(browser_context),
      render_process_id_(render_process_id),
      frame_tree_node_id_(frame_tree_node_id),
      request_id_generator_(request_id_generator),
      disconnect_callback_(std::move(on_disconnect)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(proxy_receivers_.empty());
  DCHECK(!target_factory_.is_bound());

  target_factory_.Bind(std::move(target_factory));
  target_factory_.set_connection_error_handler(
      base::BindOnce(&BraveProxyingURLLoaderFactory::OnTargetFactoryError,
                     base::Unretained(this)));

  proxy_receivers_.Add(this, std::move(receiver));
  proxy_receivers_.set_disconnect_handler(
      base::BindRepeating(&BraveProxyingURLLoaderFactory::OnProxyBindingError,
                          base::Unretained(this)));
}

BraveProxyingURLLoaderFactory::~BraveProxyingURLLoaderFactory() = default;

// static
bool BraveProxyingURLLoaderFactory::MaybeProxyRequest(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    int render_process_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto proxied_receiver = std::move(*factory_receiver);
  network::mojom::URLLoaderFactoryPtrInfo target_factory_info;
  *factory_receiver = mojo::MakeRequest(&target_factory_info);

  ResourceContextData::StartProxying(
      browser_context, render_process_id,
      render_frame_host ? render_frame_host->GetFrameTreeNodeId() : 0,
      std::move(proxied_receiver), std::move(target_factory_info));
  return true;
}

void BraveProxyingURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // The request ID doesn't really matter in the Network Service path. It just
  // needs to be unique per-BrowserContext so request handlers can make sense of
  // it. Note that |network_service_request_id_| by contrast is not necessarily
  // unique, so we don't use it for identity here.
  const uint64_t brave_request_id = request_id_generator_->Generate();

  auto result = requests_.emplace(std::make_unique<InProgressRequest>(
      this, brave_request_id, request_id, routing_id, render_process_id_,
      frame_tree_node_id_, options, request, browser_context_,
      traffic_annotation, std::move(loader_receiver), std::move(client)));
  (*result.first)->Restart();
}

void BraveProxyingURLLoaderFactory::Clone(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> loader_receiver) {
  proxy_receivers_.Add(this, std::move(loader_receiver));
}

void BraveProxyingURLLoaderFactory::OnTargetFactoryError() {
  // Stop calls to CreateLoaderAndStart() when |target_factory_| is invalid.
  target_factory_.reset();
  proxy_receivers_.Clear();
  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::OnProxyBindingError() {
  if (proxy_receivers_.empty())
    target_factory_.reset();

  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::RemoveRequest(InProgressRequest* request) {
  auto it = requests_.find(request);
  DCHECK(it != requests_.end());
  requests_.erase(it);

  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::MaybeRemoveProxy() {
  // Even if all URLLoaderFactory pipes connected to this object have been
  // closed it has to stay alive until all active requests have completed.
  if (target_factory_.is_bound() || !requests_.empty())
    return;

  // Deletes |this|.
  std::move(disconnect_callback_).Run(this);
}
