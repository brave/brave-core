/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_THROTTLE_H_
#define BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_THROTTLE_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace body_sniffer {

class BodySnifferURLLoader;

// Base throttle used for implementing sniffing functionality
class BodySnifferThrottle : public blink::URLLoaderThrottle,
                            public base::SupportsWeakPtr<BodySnifferThrottle> {
 public:
  // |task_runner| is used to bind the right task runner for handling incoming
  // IPC in BodySnifferUrlLoader. |task_runner| is supposed to be bound to the
  // current sequence.
  explicit BodySnifferThrottle(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  ~BodySnifferThrottle() override;
  BodySnifferThrottle& operator=(const BodySnifferThrottle&) = delete;

  void SetBodyProducer(std::unique_ptr<class BodyProducer> producer);
  void AddHandler(std::unique_ptr<class BodyHandler> handler);

  void Cancel();

  void Resume(network::mojom::URLResponseHeadPtr response_head,
              mojo::ScopedDataPipeConsumerHandle body);

 protected:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;

  void InterceptAndStartLoader(
      mojo::PendingRemote<network::mojom::URLLoader> new_remote,
      mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver,
      BodySnifferURLLoader* loader);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<class BodyProducer> producer_;
  std::vector<std::unique_ptr<class BodyHandler>> body_handlers_;

  base::WeakPtrFactory<BodySnifferThrottle> weak_factory_{this};
};

}  // namespace body_sniffer

#endif  // BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_THROTTLE_H_
