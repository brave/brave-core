// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "brave/components/ai_chat/renderer/yt_util.h"

namespace ai_chat {

namespace {
constexpr const char kYouTubePlayerAPIPath[] = "/youtubei/v1/player";
}

AIChatResourceSniffer::AIChatResourceSniffer(
    base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate)
    : delegate_(std::move(delegate)) {}

AIChatResourceSniffer::~AIChatResourceSniffer() = default;

// static
std::unique_ptr<AIChatResourceSniffer> AIChatResourceSniffer::MaybeCreate(
    const GURL& url,
    base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate) {
  DCHECK(delegate);

  // TODO(petemill): Allow some kind of config to be passed in to determine
  // which hosts and paths to sniff, and how to parse it to a
  // |mojom::PageContent|.
  if (url.SchemeIsHTTPOrHTTPS() &&
      base::Contains(kYouTubeHosts, url.host_piece()) &&
      base::EqualsCaseInsensitiveASCII(url.path_piece(),
                                       kYouTubePlayerAPIPath)) {
    return base::WrapUnique(new AIChatResourceSniffer(std::move(delegate)));
  }

  return nullptr;
}

bool AIChatResourceSniffer::OnRequest(network::ResourceRequest* request) {
  return true;
}

bool AIChatResourceSniffer::ShouldProcess(
    const GURL& url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  return true;
}

void AIChatResourceSniffer::OnComplete() {}

AIChatResourceSniffer::Action AIChatResourceSniffer::OnBodyUpdated(
    const std::string& body,
    bool is_complete) {
  if (!is_complete) {
    return AIChatResourceSniffer::Action::kContinue;
  }

  if (!body.empty() && delegate_) {
    auto content = std::make_unique<
        AIChatResourceSnifferThrottleDelegate::InterceptedContent>();
    content->type = AIChatResourceSnifferThrottleDelegate::
        InterceptedContentType::kYouTubeMetadataString;
    content->content = body;
    delegate_->OnInterceptedPageContentChanged(std::move(content));
  }

  return AIChatResourceSniffer::Action::kComplete;
}

bool AIChatResourceSniffer::IsTransformer() const {
  return false;
}

void AIChatResourceSniffer::Transform(
    std::string body,
    base::OnceCallback<void(std::string)> on_complete) {
  NOTREACHED_IN_MIGRATION();
}

void AIChatResourceSniffer::UpdateResponseHead(
    network::mojom::URLResponseHead* response_head) {}

}  // namespace ai_chat
