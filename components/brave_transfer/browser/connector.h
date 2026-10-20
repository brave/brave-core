// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_CONNECTOR_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_CONNECTOR_H_

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_transfer/browser/byte_stream.h"
#include "brave/components/brave_transfer/core/discovery_provider.h"

namespace brave_transfer {

// Opens outgoing connections to discovered peers.
class Connector {
 public:
  // Receives null if no endpoint could be reached.
  using ConnectCallback =
      base::OnceCallback<void(std::unique_ptr<ByteStream> stream)>;

  virtual ~Connector() = default;

  // Tries `endpoints` in order and returns the first connection that
  // succeeds.
  virtual void Connect(const std::vector<PeerEndpoint>& endpoints,
                       ConnectCallback callback) = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_CONNECTOR_H_
