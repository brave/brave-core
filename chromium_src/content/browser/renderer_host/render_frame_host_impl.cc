/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_BINDRESTRICTEDCOOKIEMANAGER                                   \
  bool should_use_ephemeral_storage =                                       \
      !frame_tree_node_->IsMainFrame() && ComputeSiteForCookies().IsNull(); \
  if (should_use_ephemeral_storage) {                                       \
    EphemeralStoragePartition* partition =                                  \
        GetBrowserContext()->GetExistingEphemeralStoragePartition(          \
            delegate_->GetMainFrameLastCommittedURL());                     \
    if (partition) {                                                        \
      partition->CreateRestrictedCookieManagerForScript(                    \
          GetLastCommittedOrigin(), isolation_info_.site_for_cookies(),     \
          ComputeTopFrameOrigin(GetLastCommittedOrigin()),                  \
          GetProcess()->GetID(), routing_id(), std::move(receiver),         \
          CreateCookieAccessObserver());                                    \
      return;                                                               \
    }                                                                       \
  }

#include "../../../../../content/browser/renderer_host/render_frame_host_impl.cc"
