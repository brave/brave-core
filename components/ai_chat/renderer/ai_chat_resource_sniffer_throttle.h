// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_THROTTLE_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_THROTTLE_H_

#include <memory>

#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_throttle_delegate.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"

namespace ai_chat {

class AIChatResourceSnifferThrottleDelegate;

// ResourceSnifferThrottle is an interceptor which reads the content of various
// resources and sends it to an AI Chat delegate for content updates.
class AIChatResourceSnifferThrottle : public body_sniffer::BodySnifferThrottle {
 public:
  explicit AIChatResourceSnifferThrottle(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate);
  ~AIChatResourceSnifferThrottle() override;
  AIChatResourceSnifferThrottle& operator=(
      const AIChatResourceSnifferThrottle&) = delete;

  static std::unique_ptr<AIChatResourceSnifferThrottle> MaybeCreateThrottleFor(
      base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate,
      const GURL& url,
      scoped_refptr<base::SequencedTaskRunner> task_runner);

 protected:
  // blink::URLLoaderThrottle via body_sniffer::BodySnifferThrottle
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;

 private:
  friend class AIChatResourceSnifferThrottleTest;
  FRIEND_TEST_ALL_PREFIXES(AIChatResourceSnifferThrottleTest, NoBody);
  FRIEND_TEST_ALL_PREFIXES(AIChatResourceSnifferThrottleTest, Body_NonJson);
  FRIEND_TEST_ALL_PREFIXES(AIChatResourceSnifferThrottleTest, Body_InvalidJson);
  FRIEND_TEST_ALL_PREFIXES(AIChatResourceSnifferThrottleTest,
                           Body_ValidNonYTJson);
  FRIEND_TEST_ALL_PREFIXES(AIChatResourceSnifferThrottleTest, Body_ValidYTJson);
  FRIEND_TEST_ALL_PREFIXES(AIChatResourceSnifferThrottleTest, Abort_NoBodyPipe);
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_THROTTLE_H_
