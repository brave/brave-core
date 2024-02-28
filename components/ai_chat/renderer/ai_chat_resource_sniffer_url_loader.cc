// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_url_loader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace ai_chat {

namespace {

constexpr uint32_t kReadBufferSize = 37000;  // average subresource size

}  // namespace

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           AIChatResourceSnifferURLLoader*>
AIChatResourceSnifferURLLoader::CreateLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const GURL& response_url) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(new AIChatResourceSnifferURLLoader(
      std::move(throttle), delegate, std::move(url_loader_client),
      std::move(task_runner), response_url));
  AIChatResourceSnifferURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr);
}

AIChatResourceSnifferURLLoader::AIChatResourceSnifferURLLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const GURL& response_url)
    : body_sniffer::BodySnifferURLLoader(
          throttle,
          response_url,
          std::move(destination_url_loader_client),
          task_runner),
      delegate_(delegate),
      response_url_(response_url) {}

AIChatResourceSnifferURLLoader::~AIChatResourceSnifferURLLoader() = default;

void AIChatResourceSnifferURLLoader::OnBodyReadable(MojoResult) {
  DCHECK_EQ(State::kLoading, state_);

  if (!BodySnifferURLLoader::CheckBufferedBody(kReadBufferSize)) {
    return;
  }

  body_consumer_watcher_.ArmOrNotify();
}

void AIChatResourceSnifferURLLoader::OnBodyWritable(MojoResult r) {
  DCHECK_EQ(State::kSending, state_);
  if (bytes_remaining_in_buffer_ > 0) {
    SendBufferedBodyToClient();
  } else {
    CompleteSending();
  }
}

void AIChatResourceSnifferURLLoader::CompleteLoading(std::string body) {
  DVLOG(4) << __func__ << ": got body length: " << body.size()
           << " for url: " << response_url_.spec();
  if (!body.empty()) {
    auto content = std::make_unique<
        AIChatResourceSnifferThrottleDelegate::InterceptedContent>();
    content->type = AIChatResourceSnifferThrottleDelegate::
        InterceptedContentType::kYouTubeMetadataString;
    content->content = body;
    delegate_->OnInterceptedPageContentChanged(std::move(content));
  }
  body_sniffer::BodySnifferURLLoader::CompleteLoading(std::move(body));
}

}  // namespace ai_chat
