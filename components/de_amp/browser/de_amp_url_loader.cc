/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_url_loader.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "brave/components/de_amp/browser/de_amp_throttle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/redirect_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace de_amp {

namespace {

constexpr uint32_t kReadBufferSize = 65536;

}  // namespace

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           DeAmpURLLoader*>
DeAmpURLLoader::CreateLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    const GURL& response_url,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    DeAmpService* service,
    network::ResourceRequest request,
    network::mojom::URLResponseHead* response,
    content::WebContents* contents) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(new DeAmpURLLoader(
      std::move(throttle), response_url, std::move(url_loader_client),
      std::move(task_runner), service, request, response, contents));
  DeAmpURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr);
}

DeAmpURLLoader::DeAmpURLLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    const GURL& response_url,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    DeAmpService* service,
    network::ResourceRequest request,
    network::mojom::URLResponseHead* response,
    content::WebContents* contents)
    : body_sniffer::BodySnifferURLLoader(
          throttle,
          response_url,
          std::move(destination_url_loader_client),
          task_runner),
      contents_(contents),
      request_(request),
      response_(response),
      de_amp_service(service) {}

DeAmpURLLoader::~DeAmpURLLoader() = default;

void DeAmpURLLoader::OnBodyReadable(MojoResult) {
  if (state_ == State::kSending) {
    // The pipe becoming readable when kSending means all buffered body has
    // already been sent.
    ForwardBodyToClient();
    return;
  }
  if (!BodySnifferURLLoader::CheckBufferedBody(kReadBufferSize)) {
    return;
  }

  MaybeRedirectToCanonicalLink();

  body_consumer_watcher_.ArmOrNotify();
}

void DeAmpURLLoader::MaybeRedirectToCanonicalLink() {
  std::string canonical_link;

  if (de_amp_service->FindCanonicalLinkIfAMP(buffered_body_, &canonical_link)) {
    const GURL canonical_url(canonical_link);
    if (!de_amp_service->VerifyCanonicalLink(canonical_url, response_url_)) {
      VLOG(2) << __func__ << " canonical link check failed " << canonical_url;
      CompleteLoading(std::move(buffered_body_));
      return;
    }
    VLOG(2) << __func__ << " de-amping and loading " << canonical_url;
    net::RedirectInfo redirect_info =
        DeAmpURLLoader::CreateRedirectInfo(canonical_url, request_, *response_);
    destination_url_loader_client_->OnReceiveRedirect(
        redirect_info, DeAmpURLLoader::CreateRedirectResponseHead(*response_));
    // Abort();
    return;
    // contents_->GetController().LoadURL(canonical_url, content::Referrer(),
    //                                    ui::PAGE_TRANSITION_CLIENT_REDIRECT,
    //                                    std::string());
  } else {
    // Did not find AMP page and/or canonical link, load original
    CompleteLoading(std::move(buffered_body_));
    return;
  }
}

network::mojom::URLResponseHeadPtr DeAmpURLLoader::CreateRedirectResponseHead(
    const network::mojom::URLResponseHead& outer_response) {
  auto response_head = network::mojom::URLResponseHead::New();
  response_head->encoded_data_length = 0;
  std::string buf;
  std::string link_header;
  outer_response.headers->GetNormalizedHeader("link", &link_header);
  if (link_header.empty()) {
    buf = base::StringPrintf("HTTP/1.1 %d %s\r\n", 307, "Internal Redirect");
  } else {
    buf = base::StringPrintf(
        "HTTP/1.1 %d %s\r\n"
        "link: %s\r\n",
        307, "Internal Redirect", link_header.c_str());
  }
  response_head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(buf));
  response_head->encoded_data_length = 0;
  response_head->request_start = outer_response.request_start;
  response_head->response_start = outer_response.response_start;
  response_head->request_time = outer_response.request_time;
  response_head->response_time = outer_response.response_time;
  response_head->load_timing = outer_response.load_timing;
  return response_head;
}

net::RedirectInfo DeAmpURLLoader::CreateRedirectInfo(
    const GURL& new_url,
    const network::ResourceRequest& outer_request,
    const network::mojom::URLResponseHead& outer_response) {
  return net::RedirectInfo::ComputeRedirectInfo(
      "GET", outer_request.url, outer_request.site_for_cookies,
      outer_request.update_first_party_url_on_redirect
          ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
          : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL,
      outer_request.referrer_policy, outer_request.referrer.spec(), 307,
      new_url,
      net::RedirectUtil::GetReferrerPolicyHeader(outer_response.headers.get()),
      false /* insecure_scheme_was_upgraded */, true /* copy_fragment */,
      false);
}

void DeAmpURLLoader::OnBodyWritable(MojoResult r) {
  DCHECK_EQ(State::kSending, state_);
  if (bytes_remaining_in_buffer_ > 0) {
    SendReceivedBodyToClient();
  } else {
    ForwardBodyToClient();
  }
}

void DeAmpURLLoader::ForwardBodyToClient() {
  DCHECK_EQ(0u, bytes_remaining_in_buffer_);
  // Send the body from the consumer to the producer.
  const void* buffer;
  uint32_t buffer_size = 0;
  MojoResult result = body_consumer_handle_->BeginReadData(
      &buffer, &buffer_size, MOJO_BEGIN_READ_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_watcher_.ArmOrNotify();
      return;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // All data has been sent.
      CompleteSending();
      return;
    default:
      NOTREACHED();
      return;
  }

  result = body_producer_handle_->WriteData(buffer, &buffer_size,
                                            MOJO_WRITE_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // The pipe is closed unexpectedly. |this| should be deleted once
      // URLLoader on the destination is released.
      BodySnifferURLLoader::Abort();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_handle_->EndReadData(0);
      body_producer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }

  body_consumer_handle_->EndReadData(buffer_size);
  body_consumer_watcher_.ArmOrNotify();
}

void DeAmpURLLoader::CompleteSending() {
  DCHECK_EQ(State::kSending, state_);
  state_ = State::kCompleted;
  // Call client's OnComplete() if |this|'s OnComplete() has already been
  // called.
  if (complete_status_.has_value()) {
    destination_url_loader_client_->OnComplete(complete_status_.value());
  }

  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  body_consumer_handle_.reset();
  body_producer_handle_.reset();
}

}  // namespace de_amp
