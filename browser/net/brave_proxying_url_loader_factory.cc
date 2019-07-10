/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include <memory>
#include <utility>

// TODO(iefremov): cleanup headers.
#include "base/bind.h"
#include "base/debug/alias.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/browser/net/brave_shields_core.h"
#include "brave/components/brave_shields/browser/adblock_stub_response.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/url_utils.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "extensions/common/manifest_handlers/web_accessible_resources_info.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/completion_repeating_callback.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/platform/resource_request_blocked_reason.h"
#include "url/origin.h"

namespace {

// User data key for ResourceContextData.
const void* const kResourceContextUserDataKey = &kResourceContextUserDataKey;

class ResourceContextData : public base::SupportsUserData::Data {
 public:
  ~ResourceContextData() override {}

  static void StartProxying(
      content::ResourceContext* resource_context,
      int render_process_id,
      int frame_tree_node_id,
      bool is_download,
      network::mojom::URLLoaderFactoryRequest request,
      network::mojom::URLLoaderFactoryPtrInfo target_factory,
      network::mojom::TrustedURLLoaderHeaderClientRequest header_client_request) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

    auto* self = static_cast<ResourceContextData*>(
        resource_context->GetUserData(kResourceContextUserDataKey));
    if (!self) {
      self = new ResourceContextData();
      resource_context->SetUserData(kResourceContextUserDataKey,
                                    base::WrapUnique(self));
    }

    auto proxy = std::make_unique<BraveProxyingURLLoaderFactory>(
        resource_context,
        render_process_id,
        frame_tree_node_id,
        is_download,
        std::move(request),
        std::move(target_factory),
        std::move(header_client_request),
        base::BindOnce(&ResourceContextData::RemoveProxy,
                       self->weak_factory_.GetWeakPtr()));

    self->proxies_.emplace(std::move(proxy));
  }

  void RemoveProxy(BraveProxyingURLLoaderFactory* proxy) {
    auto it = proxies_.find(proxy);
    DCHECK(it != proxies_.end());
    proxies_.erase(it);
  }

 private:
  ResourceContextData() : weak_factory_(this) {}

  std::set<std::unique_ptr<BraveProxyingURLLoaderFactory>, base::UniquePtrComparator>
      proxies_;

  base::WeakPtrFactory<ResourceContextData> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ResourceContextData);
};

// Helper struct for crafting responses.
struct WriteData {
  // Wek ref. |client| destroys itself in |OnComplete()|.
  network::mojom::URLLoaderClient* client;
  std::string data;
  std::unique_ptr<mojo::StringDataPipeProducer> producer;
};

void OnWrite(std::unique_ptr<WriteData> write_data, MojoResult result) {
  if (result != MOJO_RESULT_OK) {
    network::URLLoaderCompletionStatus status(net::ERR_FAILED);
    write_data->client->OnComplete(status);
    return;
  }

  network::URLLoaderCompletionStatus status(net::OK);
  status.encoded_data_length = write_data->data.size();
  status.encoded_body_length = write_data->data.size();
  status.decoded_body_length = write_data->data.size();
  write_data->client->OnComplete(status);
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
    bool is_download,
    content::ResourceContext* resource_context,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    network::mojom::URLLoaderRequest loader_request,
    network::mojom::URLLoaderClientPtr client)
    : factory_(factory),
      request_(request),
      original_initiator_(request.request_initiator),
      // is_download_(is_download),
      request_id_(request_id),
      network_service_request_id_(network_service_request_id),
      render_process_id_(render_process_id),
      frame_tree_node_id_(frame_tree_node_id),
      routing_id_(routing_id),
      options_(options),
      resource_context_(resource_context),
      traffic_annotation_(traffic_annotation),
      proxied_loader_binding_(this, std::move(loader_request)),
      target_client_(std::move(client)),
      proxied_client_binding_(this),
// TODO(iefremov):
//      has_any_extra_headers_listeners_(
//          network_service_request_id_ != 0 &&
//          ExtensionWebRequestEventRouter::GetInstance()
//              ->HasAnyExtraHeadersListener(factory_->browser_context_)),
      header_client_binding_(this),
      weak_factory_(this) {
  // If there is a client error, clean up the request.
  target_client_.set_connection_error_handler(base::BindOnce(
      &BraveProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
      weak_factory_.GetWeakPtr(),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED)));
}

BraveProxyingURLLoaderFactory::InProgressRequest::~InProgressRequest() {
  // This is important to ensure that no outstanding blocking requests continue
  // to reference state owned by this object.

// TODO(iefremov):
//  if (info_) {
//    ExtensionWebRequestEventRouter::GetInstance()->OnRequestWillBeDestroyed(
//        factory_->browser_context_, &info_.value());
//  }
  if (on_before_send_headers_callback_) {
    std::move(on_before_send_headers_callback_)
        .Run(net::ERR_ABORTED, base::nullopt);
  }
  if (on_headers_received_callback_) {
    std::move(on_headers_received_callback_)
        .Run(net::ERR_ABORTED, base::nullopt, GURL());
  }
}

void BraveProxyingURLLoaderFactory::InProgressRequest::Restart() {
  UpdateRequestInfo();
  RestartInternal();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    UpdateRequestInfo() {
  // Derive a new WebRequestInfo value any time |Restart()| is called, because
  // the details in |request_| may have changed e.g. if we've been redirected.
  // |request_initiator| can be modified on redirects, but we keep the original
  // for |initiator| in the event. See also
  // https://developer.chrome.com/extensions/webRequest#event-onBeforeRequest.
  network::ResourceRequest request_for_info = request_;
  request_for_info.request_initiator = original_initiator_;

// TODO(iefremov):
//  info_.emplace(
//      request_id_, factory_->render_process_id_, request_.render_frame_id,
//      factory_->navigation_ui_data_ ? factory_->navigation_ui_data_->DeepCopy()
//                                    : nullptr,
//      routing_id_, factory_->resource_context_, request_for_info, is_download_,
//      !(options_ & network::mojom::kURLLoadOptionSynchronous));

// _CALL_
//  current_request_uses_header_client_ =
//      factory_->url_loader_header_client_binding_ &&
//      request_.url.SchemeIsHTTPOrHTTPS() && network_service_request_id_ != 0 &&
//      ExtensionWebRequestEventRouter::GetInstance()
//          ->HasExtraHeadersListenerForRequest(
//              factory_->browser_context_, factory_->info_map_, &info_.value());
}

void BraveProxyingURLLoaderFactory::InProgressRequest::RestartInternal() {
  // TODO(iefremov):
//  DCHECK_EQ(info_->url, request_.url)
//      << "UpdateRequestInfo must have been called first";
  request_completed_ = false;

  // If the header client will be used, we start the request immediately, and
  // OnBeforeSendHeaders and OnSendHeaders will be handled there. Otherwise,
  // send these events before the request starts.
  base::RepeatingCallback<void(int)> continuation;
  if (current_request_uses_header_client_) {
    continuation = base::BindRepeating(
        &InProgressRequest::ContinueToStartRequest, weak_factory_.GetWeakPtr());
  } else {
    continuation =
        base::BindRepeating(&InProgressRequest::ContinueToBeforeSendHeaders,
                            weak_factory_.GetWeakPtr());
  }
  redirect_url_ = GURL();
  bool should_collapse_initiator = false;  // TODO(iefremov): Check this
  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, render_process_id_,
                                   frame_tree_node_id_, request_id_,
                                   resource_context_, ctx_);
  int result = BraveShieldsCore::GetInstance()->OnBeforeURLRequest(
      ctx_,
      continuation,
      &redirect_url_);

  // TODO(iefremov): DCHECK?
  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // The request was cancelled synchronously. Dispatch an error notification
    // and terminate the request.
    network::URLLoaderCompletionStatus status(result);
    if (should_collapse_initiator) {
      status.extended_error_code = static_cast<int>(
          blink::ResourceRequestBlockedReason::kCollapsedByClient);
    }
    OnRequestError(status);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // One or more listeners is blocking, so the request must be paused until
    // they respond. |continuation| above will be invoked asynchronously to
    // continue or cancel the request.
    //
    // We pause the binding here to prevent further client message processing.
    if (proxied_client_binding_.is_bound())
      proxied_client_binding_.PauseIncomingMethodCallProcessing();

    // Pause the header client, since we want to wait until OnBeforeRequest has
    // finished before processing any future events.
    if (header_client_binding_)
      header_client_binding_.PauseIncomingMethodCallProcessing();
    return;
  }
  DCHECK_EQ(net::OK, result);

  continuation.Run(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const base::Optional<GURL>& new_url) {
  if (new_url)
    request_.url = new_url.value();

  for (const std::string& header : removed_headers)
    request_.headers.RemoveHeader(header);
  request_.headers.MergeFrom(modified_headers);

  // Call this before checking |current_request_uses_header_client_| as it
  // calculates it.
  UpdateRequestInfo();

  if (target_loader_.is_bound()) {
    // If header_client_ is used, then we have to call FollowRedirect now as
    // that's what triggers the network service calling back to
    // OnBeforeSendHeaders(). Otherwise, don't call FollowRedirect now. Wait for
    // the onBeforeSendHeaders callback(s) to run as these may modify request
    // headers and if so we'll pass these modifications to FollowRedirect.
    if (current_request_uses_header_client_) {
      target_loader_->FollowRedirect(removed_headers, modified_headers,
                                     new_url);
    } else {
      auto params = std::make_unique<FollowRedirectParams>();
      params->removed_headers = removed_headers;
      params->modified_headers = modified_headers;
      params->new_url = new_url;
      pending_follow_redirect_params_ = std::move(params);
    }
  }

  RestartInternal();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ProceedWithResponse() {
  if (target_loader_.is_bound())
    target_loader_->ProceedWithResponse();
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
    const network::ResourceResponseHead& head) {
  on_receive_response_received_ = true;
  if (current_request_uses_header_client_) {
    // Use the headers we got from OnHeadersReceived as that'll contain
    // Set-Cookie if it existed.
    auto saved_headers = current_response_.headers;
    current_response_ = head;
    current_response_.headers = saved_headers;
    ContinueToResponseStarted(net::OK);
  } else {
    current_response_ = head;
    HandleResponseOrRedirectHeaders(
        base::BindRepeating(&InProgressRequest::ContinueToResponseStarted,
                            weak_factory_.GetWeakPtr()));
  }
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& head) {
  if (redirect_url_ != redirect_info.new_url &&
      !IsRedirectSafe(request_.url, redirect_info.new_url)) {
    OnRequestError(
        network::URLLoaderCompletionStatus(net::ERR_UNSAFE_REDIRECT));
    return;
  }

  if (current_request_uses_header_client_) {
    // Use the headers we got from OnHeadersReceived as that'll contain
    // Set-Cookie if it existed.
    auto saved_headers = current_response_.headers;
    current_response_ = head;
    current_response_.headers = saved_headers;
    ContinueToBeforeRedirect(redirect_info, net::OK);
  } else {
    current_response_ = head;
    HandleResponseOrRedirectHeaders(
        base::BindRepeating(&InProgressRequest::ContinueToBeforeRedirect,
                            weak_factory_.GetWeakPtr(), redirect_info));
  }
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    OnReceiveCachedMetadata(const std::vector<uint8_t>& data) {
  target_client_->OnReceiveCachedMetadata(data);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    OnTransferSizeUpdated(int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    OnStartLoadingResponseBody(mojo::ScopedDataPipeConsumerHandle body) {
  // TODO(https://crbug.com/882661): Remove this once the bug is fixed.
  if (!on_receive_response_sent_) {
    bool on_receive_response_received = on_receive_response_received_;
    base::debug::Alias(&on_receive_response_received);
    DEBUG_ALIAS_FOR_GURL(request_url, request_.url)
    base::debug::DumpWithoutCrashing();
  }
  target_client_->OnStartLoadingResponseBody(std::move(body));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  if (status.error_code != net::OK) {
    OnRequestError(status);
    return;
  }

  target_client_->OnComplete(status);
// TODO(iefremov):
//  ExtensionWebRequestEventRouter::GetInstance()->OnCompleted(
//      factory_->browser_context_, factory_->info_map_, &info_.value(),
//      status.error_code);

  // Deletes |this|.
  factory_->RemoveRequest(this);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnLoaderCreated(
    network::mojom::TrustedHeaderClientRequest request) {
  header_client_binding_.Bind(std::move(request));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  if (!current_request_uses_header_client_) {
    std::move(callback).Run(net::OK, base::nullopt);
    return;
  }

  request_.headers = headers;
  on_before_send_headers_callback_ = std::move(callback);
  ContinueToBeforeSendHeaders(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::OnHeadersReceived(
    const std::string& headers,
    OnHeadersReceivedCallback callback) {
  if (!current_request_uses_header_client_) {
    std::move(callback).Run(net::OK, base::nullopt, GURL());
    return;
  }

  on_headers_received_callback_ = std::move(callback);
  current_response_.headers =
      base::MakeRefCounted<net::HttpResponseHeaders>(headers);
  HandleResponseOrRedirectHeaders(
      base::BindRepeating(&InProgressRequest::ContinueToHandleOverrideHeaders,
                          weak_factory_.GetWeakPtr()));
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    HandleBeforeRequestRedirect() {
  // The extension requested a redirect. Close the connection with the current
  // URLLoader and inform the URLLoaderClient the WebRequest API generated a
  // redirect. To load |redirect_url_|, a new URLLoader will be recreated
  // after receiving FollowRedirect().

  // Forgetting to close the connection with the current URLLoader caused
  // bugs. The latter doesn't know anything about the redirect. Continuing
  // the load with it gives unexpected results. See
  // https://crbug.com/882661#c72.
  proxied_client_binding_.Close();
  header_client_binding_.Close();
  target_loader_.reset();

  constexpr int kInternalRedirectStatusCode = 307;

  net::RedirectInfo redirect_info;
  redirect_info.status_code = kInternalRedirectStatusCode;
  redirect_info.new_method = request_.method;
  redirect_info.new_url = redirect_url_;
  redirect_info.new_site_for_cookies = redirect_url_;

  network::ResourceResponseHead head;
  std::string headers = base::StringPrintf(
      "HTTP/1.1 %i Internal Redirect\n"
      "Location: %s\n"
      "Non-Authoritative-Reason: WebRequest API\n\n",
      kInternalRedirectStatusCode, redirect_url_.spec().c_str());

  if (base::FeatureList::IsEnabled(network::features::kOutOfBlinkCors)) {
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
  } else {
    // If this redirect is used in a cross-origin request, add CORS headers to
    // make sure that the redirect gets through the Blink CORS. Note that the
    // destination URL is still subject to the usual CORS policy, i.e. the
    // resource will only be available to web pages if the server serves the
    // response with the required CORS response headers. Matches the behavior in
    // url_request_redirect_job.cc.
    std::string http_origin;
    if (request_.headers.GetHeader("Origin", &http_origin)) {
      headers += base::StringPrintf(
          "\n"
          "Access-Control-Allow-Origin: %s\n"
          "Access-Control-Allow-Credentials: true",
          http_origin.c_str());
    }
  }
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.length()));
  head.encoded_data_length = 0;

  current_response_ = head;
  ContinueToBeforeRedirect(redirect_info, net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToBeforeSendHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (!current_request_uses_header_client_ && !redirect_url_.is_empty()) {
    HandleBeforeRequestRedirect();
    return;
  }
  DCHECK(ctx_);
  if (!ctx_->new_referrer.is_empty()) {
    request_.referrer = ctx_->new_referrer;
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  if (ctx_->blocked_by != brave::kNotBlocked) {
    if (ctx_->cancel_request_explicitly) {
      OnRequestError(network::URLLoaderCompletionStatus(net::ERR_ABORTED));
      return;
    }
    network::ResourceResponseHead response;
    std::string response_data;
    brave_shields::MakeStubResponse(request_, &response,
                                    &response_data);

    target_client_->OnReceiveResponse(response);

    // Create a data pipe for transmitting the response.
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (CreateDataPipe(nullptr, &producer, &consumer) != MOJO_RESULT_OK) {
      OnRequestError(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }

    // Craft the response.
    target_client_->OnStartLoadingResponseBody(std::move(consumer));

    auto write_data = std::make_unique<WriteData>();
    write_data->client = this;
    write_data->data = response_data;
    write_data->producer =
        std::make_unique<mojo::StringDataPipeProducer>(std::move(producer));

    base::StringPiece string_piece(write_data->data);
    write_data->producer->Write(string_piece,
                                mojo::StringDataPipeProducer::AsyncWritingMode::
                                    STRING_STAYS_VALID_UNTIL_COMPLETION,
                                base::BindOnce(OnWrite, std::move(write_data)));
    return;
  }

  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    // NOTE: While it does not appear to be documented (and in fact it may be
    // intuitive), |onBeforeSendHeaders| is only dispatched for HTTP and HTTPS
    // requests.
    // TODO(iefremov): ^^^ Is it relevant for us?
    // TODO(iefremov): Probably we will need real headers?

    auto continuation = base::BindRepeating(
        &InProgressRequest::ContinueToSendHeaders,
        weak_factory_.GetWeakPtr(),
        std::set<std::string>(),
        std::set<std::string>());

    ctx_ = std::make_shared<brave::BraveRequestInfo>();
    brave::BraveRequestInfo::FillCTX(request_, render_process_id_,
                                     frame_tree_node_id_,
                                     request_id_, resource_context_, ctx_);
    int result = BraveShieldsCore::GetInstance()->OnBeforeStartTransaction(
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
      if (proxied_client_binding_.is_bound())
        proxied_client_binding_.PauseIncomingMethodCallProcessing();
      return;
    }
    DCHECK_EQ(net::OK, result);
  }

  ContinueToSendHeaders(std::set<std::string>(), std::set<std::string>(),
                        net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToStartRequest(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (current_request_uses_header_client_ && !redirect_url_.is_empty()) {
    HandleBeforeRequestRedirect();
    return;
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  if (header_client_binding_)
    header_client_binding_.ResumeIncomingMethodCallProcessing();

  if (!target_loader_.is_bound() && factory_->target_factory_.is_bound()) {
    // No extensions have cancelled us up to this point, so it's now OK to
    // initiate the real network request.
    network::mojom::URLLoaderClientPtr proxied_client;
    proxied_client_binding_.Bind(mojo::MakeRequest(&proxied_client));
    uint32_t options = options_;
    // Even if this request does not use the header client, future redirects
    // might, so we need to set the option on the loader.
    if (has_any_extra_headers_listeners_)
      options |= network::mojom::kURLLoadOptionUseHeaderClient;
    factory_->target_factory_->CreateLoaderAndStart(
        // TODO: (info_->routing_id)?
        mojo::MakeRequest(&target_loader_), routing_id_,
        network_service_request_id_, options, request_,
        std::move(proxied_client), traffic_annotation_);
  }

  // From here the lifecycle of this request is driven by subsequent events on
  // either |proxy_loader_binding_|, |proxy_client_binding_|, or
  // |header_client_binding_|.
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToSendHeaders(const std::set<std::string>& removed_headers,
                          const std::set<std::string>& set_headers,
                          int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (current_request_uses_header_client_) {
    DCHECK(on_before_send_headers_callback_);
    std::move(on_before_send_headers_callback_)
        .Run(error_code, request_.headers);
  } else if (pending_follow_redirect_params_) {
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
          pending_follow_redirect_params_->new_url);
    }

    pending_follow_redirect_params_.reset();
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    // NOTE: While it does not appear to be documented (and in fact it may be
    // intuitive), |onSendHeaders| is only dispatched for HTTP and HTTPS
    // requests.
// // TODO(iefremov):
//    ExtensionWebRequestEventRouter::GetInstance()->OnSendHeaders(
//        factory_->browser_context_, factory_->info_map_, &info_.value(),
//        request_.headers);
  }

  if (!current_request_uses_header_client_)
    ContinueToStartRequest(net::OK);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToHandleOverrideHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  DCHECK(on_headers_received_callback_);
  base::Optional<std::string> headers;
  if (override_headers_) {
    headers = override_headers_->raw_headers();
    if (current_request_uses_header_client_) {
      // Make sure to update current_response_,  since when OnReceiveResponse
      // is called we will not use its headers as it might be missing the
      // Set-Cookie line (as that gets stripped over IPC).
      current_response_.headers = override_headers_;
    }
  }
  std::move(on_headers_received_callback_).Run(net::OK, headers, redirect_url_);
  override_headers_ = nullptr;

  if (proxied_client_binding_)
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToResponseStarted(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  DCHECK(!current_request_uses_header_client_ || !override_headers_);
  if (override_headers_)
    current_response_.headers = override_headers_;

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

    net::RedirectInfo redirect_info;
    redirect_info.status_code = override_headers_->response_code();
    redirect_info.new_method = request_.method;
    redirect_info.new_url = new_url;
    redirect_info.new_site_for_cookies = new_url;

    // These will get re-bound if a new request is initiated by
    // |FollowRedirect()|.
    proxied_client_binding_.Close();
    header_client_binding_.Close();
    target_loader_.reset();
    on_receive_response_received_ = false;

    ContinueToBeforeRedirect(redirect_info, net::OK);
    return;
  }

  // TODO(iefremov):
//  info_->AddResponseInfoFromResourceResponse(current_response_);

  proxied_client_binding_.ResumeIncomingMethodCallProcessing();
  on_receive_response_sent_ = true;
  target_client_->OnReceiveResponse(current_response_);
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    ContinueToBeforeRedirect(const net::RedirectInfo& redirect_info,
                             int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }
  // TODO(iefremov):
//  info_->AddResponseInfoFromResourceResponse(current_response_);

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  // TODO(iefremov):
//  ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRedirect(
//      factory_->browser_context_, factory_->info_map_, &info_.value(),
//      redirect_info.new_url);
  target_client_->OnReceiveRedirect(redirect_info, current_response_);
  request_.url = redirect_info.new_url;
  request_.method = redirect_info.new_method;
  request_.site_for_cookies = redirect_info.new_site_for_cookies;
  request_.referrer = GURL(redirect_info.new_referrer);
  request_.referrer_policy = redirect_info.new_referrer_policy;

  // The request method can be changed to "GET". In this case we need to
  // reset the request body manually.
  if (request_.method == net::HttpRequestHeaders::kGetMethod)
    request_.request_body = nullptr;

  request_completed_ = true;
}

void BraveProxyingURLLoaderFactory::InProgressRequest::
    HandleResponseOrRedirectHeaders(
        net::CompletionOnceCallback continuation) {
  override_headers_ = nullptr;
  redirect_url_ = GURL();

  net::CompletionRepeatingCallback copyable_callback =
      base::AdaptCallbackForRepeating(std::move(continuation));
  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    ctx_ = std::make_shared<brave::BraveRequestInfo>();
    brave::BraveRequestInfo::FillCTX(request_, render_process_id_,
                                     frame_tree_node_id_, request_id_,
                                     resource_context_, ctx_);
    int result = BraveShieldsCore::GetInstance()->OnHeadersReceived(
        ctx_,
        copyable_callback,
        current_response_.headers.get(),
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
      proxied_client_binding_.PauseIncomingMethodCallProcessing();
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

// Determines whether it is safe to redirect from |from_url| to |to_url|.
bool BraveProxyingURLLoaderFactory::InProgressRequest::IsRedirectSafe(
    const GURL& from_url,
    const GURL& to_url) {
  return content::IsSafeRedirectTarget(from_url, to_url);
}

BraveProxyingURLLoaderFactory::BraveProxyingURLLoaderFactory(
    content::ResourceContext* resource_context,
    int render_process_id,
    int frame_tree_node_id,
    bool is_download,
    network::mojom::URLLoaderFactoryRequest loader_request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory,
    network::mojom::TrustedURLLoaderHeaderClientRequest header_client_request,
    DisconnectCallback on_disconnect) :
    resource_context_(resource_context),
    render_process_id_(render_process_id),
    frame_tree_node_id_(frame_tree_node_id),
    is_download_(is_download),
    url_loader_header_client_binding_(this),
    disconnect_callback_(std::move(on_disconnect)),
    weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(proxy_bindings_.empty());
  DCHECK(!target_factory_.is_bound());

  target_factory_.Bind(std::move(target_factory));
  target_factory_.set_connection_error_handler(base::BindOnce(
      &BraveProxyingURLLoaderFactory::OnTargetFactoryError, base::Unretained(this)));

  proxy_bindings_.AddBinding(this, std::move(loader_request));
  proxy_bindings_.set_connection_error_handler(base::BindRepeating(
      &BraveProxyingURLLoaderFactory::OnProxyBindingError, base::Unretained(this)));

  if (header_client_request)
    url_loader_header_client_binding_.Bind(std::move(header_client_request));
}

BraveProxyingURLLoaderFactory::~BraveProxyingURLLoaderFactory() = default;

// static
bool BraveProxyingURLLoaderFactory::MaybeProxyRequest(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    int render_process_id,
    bool is_download,
    const url::Origin& request_initiator,
    network::mojom::URLLoaderFactoryRequest* factory_request,
    network::mojom::TrustedURLLoaderHeaderClientPtrInfo* header_client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto proxied_request = std::move(*factory_request);
  network::mojom::URLLoaderFactoryPtrInfo target_factory_info;
  *factory_request = mojo::MakeRequest(&target_factory_info);

  network::mojom::TrustedURLLoaderHeaderClientRequest header_client_request;
  if (header_client)
    header_client_request = mojo::MakeRequest(header_client);
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&ResourceContextData::StartProxying,
                     browser_context->GetResourceContext(),
                     render_process_id,
                     render_frame_host ?
                        render_frame_host->GetFrameTreeNodeId() : 0,
                     is_download,
                     std::move(proxied_request),
                     std::move(target_factory_info),
                     std::move(header_client_request)));
  return true;
}

void BraveProxyingURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Make sure we are not proxying a browser initiated non-navigation request.
  // TODO(iefremov):
  // DCHECK(render_process_id_ != -1 || navigation_ui_data_);

  // The request ID doesn't really matter in the Network Service path. It just
  // needs to be unique per-BrowserContext so extensions can make sense of it.
  // Note that |network_service_request_id_| by contrast is not necessarily
  // unique, so we don't use it for identity here.
  // TODO(iefremov):
  // const uint64_t web_request_id = 0; //request_id_generator_->Generate();
  static uint64_t web_request_id = 0;
  web_request_id++;

  if (request_id) {
    // Only requests with a non-zero request ID can have their proxy associated
    // with said ID. This is necessary to support correlation against any auth
    // events received by the browser. Requests with a request ID of 0 therefore
    // do not support dispatching |WebRequest.onAuthRequired| events.

    // TODO(iefremov):
//    proxies_->AssociateProxyWithRequestId(
//        this, content::GlobalRequestID(render_process_id_, request_id));
//    network_request_id_to_web_request_id_.emplace(request_id, web_request_id);
  }

  auto result = requests_.emplace(
                          std::make_unique<InProgressRequest>(
                          this, web_request_id, request_id,
                          routing_id, render_process_id_, frame_tree_node_id_,
                          options,
                          request, is_download_, resource_context_,
                          traffic_annotation,
                          std::move(loader_request), std::move(client)));
  result.first->get()->Restart();
}

void BraveProxyingURLLoaderFactory::OnLoaderCreated(
    int32_t request_id,
    network::mojom::TrustedHeaderClientRequest request) {
  // TODO(iefremov):
//  auto it = network_request_id_to_web_request_id_.find(request_id);
//  if (it == network_request_id_to_web_request_id_.end())
//    return;

//  auto request_it = requests_.find(it->second);
//  DCHECK(request_it != requests_.end());
//  request_it->second->OnLoaderCreated(std::move(request));
}

void BraveProxyingURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest loader_request) {
  proxy_bindings_.AddBinding(this, std::move(loader_request));
}

void BraveProxyingURLLoaderFactory::OnTargetFactoryError() {
  // Stop calls to CreateLoaderAndStart() when |target_factory_| is invalid.
  target_factory_.reset();
  proxy_bindings_.CloseAllBindings();
  MaybeRemoveProxy();
}

void BraveProxyingURLLoaderFactory::OnProxyBindingError() {
  if (proxy_bindings_.empty())
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
