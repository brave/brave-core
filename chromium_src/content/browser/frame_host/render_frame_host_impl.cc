/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/ephemeral_storage_partition.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

#define BRAVE_BINDRESTRICTEDCOOKIEMANAGER                                     \
  bool should_use_ephemeral_storage =                                         \
      !frame_tree_node_->IsMainFrame() && ComputeSiteForCookies().IsNull() && \
      base::FeatureList::IsEnabled(blink::features::kBraveEphemeralStorage);  \
  if (should_use_ephemeral_storage) {                                         \
    EphemeralStoragePartition* partition =                                    \
        GetBrowserContext()->GetEphemeralStoragePartitionForMainFrameURL(     \
            GetSiteInstance(), delegate_->GetMainFrameLastCommittedURL());    \
    partition->CreateRestrictedCookieManagerForScript(                        \
        GetLastCommittedOrigin(), isolation_info_.site_for_cookies(),         \
        ComputeTopFrameOrigin(GetLastCommittedOrigin()),                      \
        GetProcess()->GetID(), routing_id(), std::move(receiver),             \
        CreateCookieAccessObserver());                                        \
    return;                                                                   \
  }

#include "../../../../../content/browser/frame_host/render_frame_host_impl.cc"
