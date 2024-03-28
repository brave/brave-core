// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_URL_LOADER_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_URL_LOADER_H_

#include <string>
#include <tuple>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_throttle_delegate.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"

namespace body_sniffer {
class BodySnifferThrottle;
}  // namespace body_sniffer

namespace ai_chat {

class AIChatResourceSnifferURLLoader
    : public body_sniffer::BodySnifferURLLoader {
 public:
  ~AIChatResourceSnifferURLLoader() override;

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    AIChatResourceSnifferURLLoader*>
  CreateLoader(base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
               base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate,
               scoped_refptr<base::SequencedTaskRunner> task_runner,
               const GURL& response_url);

 private:
  AIChatResourceSnifferURLLoader(
      base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
      base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate,
      mojo::PendingRemote<network::mojom::URLLoaderClient>
          destination_url_loader_client,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      const GURL& response_url);

  // body_sniffer::BodySnifferURLLoader
  void OnBodyReadable(MojoResult) override;
  void OnBodyWritable(MojoResult) override;

  void CompleteLoading(std::string body) override;
  base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate_;

  GURL response_url_;

  base::WeakPtrFactory<AIChatResourceSnifferURLLoader> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_URL_LOADER_H_
