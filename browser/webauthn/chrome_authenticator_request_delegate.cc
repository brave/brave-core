/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "device/fido/network_context_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"

// An off-the-record frame's caBLE tunnel goes through the frame's own storage
// partition, so it stays in that profile's network context and, in a Tor
// window, goes through Tor. Regular profiles keep the system network context.
std::optional<device::NetworkContextFactory>
BraveMaybeGetOffTheRecordCableNetworkContextFactory(
    content::GlobalRenderFrameHostId render_frame_host_id) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);
  if (!render_frame_host ||
      !render_frame_host->GetBrowserContext()->IsOffTheRecord()) {
    return std::nullopt;
  }
  return base::BindRepeating(
      [](content::GlobalRenderFrameHostId render_frame_host_id)
          -> network::mojom::NetworkContext* {
        // The delegate requires its frame to outlive it, and the caBLE
        // discovery that runs this factory belongs to the delegate's request.
        content::RenderFrameHost* render_frame_host =
            content::RenderFrameHost::FromID(render_frame_host_id);
        CHECK(render_frame_host);
        return render_frame_host->GetStoragePartition()->GetNetworkContext();
      },
      render_frame_host_id);
}
