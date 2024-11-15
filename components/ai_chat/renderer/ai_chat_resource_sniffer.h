// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_H_

#include <memory>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_throttle_delegate.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"

class GURL;
namespace network {
namespace mojom {
class URLResponseHead;
}  // namespace mojom
struct ResourceRequest;
}  // namespace network

namespace ai_chat {
class AIChatResourceSnifferThrottleDelegate;

class AIChatResourceSniffer : public body_sniffer::BodyHandler {
 public:
  ~AIChatResourceSniffer() override;

  static std::unique_ptr<AIChatResourceSniffer> MaybeCreate(
      const GURL& url,
      base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate);

 private:
  explicit AIChatResourceSniffer(
      base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate);

  // body_sniffer::BodyHandler:
  bool OnRequest(network::ResourceRequest* request) override;
  bool ShouldProcess(const GURL& response_url,
                     network::mojom::URLResponseHead* response_head,
                     bool* defer) override;
  void OnComplete() override;
  Action OnBodyUpdated(const std::string& body, bool is_complete) override;
  bool IsTransformer() const override;
  void Transform(std::string body,
                 base::OnceCallback<void(std::string)> on_complete) override;
  void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) override;

  base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_H_
