/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SNIFFER_SNIFFER_THROTTLE_H_
#define BRAVE_COMPONENTS_SNIFFER_SNIFFER_THROTTLE_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/sniffer/sniffer_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

class HostContentSettingsMap;

namespace sniffer {

// Base throttle used for implementing sniffing functionality
class SnifferThrottle : public blink::URLLoaderThrottle {
 public:
  SnifferThrottle();

  ~SnifferThrottle() override;

  void Resume();

 protected:
  base::WeakPtrFactory<SnifferThrottle> weak_factory_{this};

  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override = 0;

  void InterceptAndStartLoader(
      mojo::PendingRemote<network::mojom::URLLoader> source_loader,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>
          source_client_receiver,
      mojo::PendingRemote<network::mojom::URLLoader> new_remote,
      mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver,
      SnifferURLLoader* loader,
      bool* defer);
};

}  // namespace sniffer

#endif  // BRAVE_COMPONENTS_SNIFFER_SNIFFER_THROTTLE_H_
