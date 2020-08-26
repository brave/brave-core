/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/browser_context.h"

#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/storage_partition_impl_map.h"

#include "../../../../../content/browser/browser_context.cc"

namespace content {

void BrowserContext::ClearEphemeralStorageForHost(RenderViewHost* host,
                                                  SiteInstance* site_instance) {
  StoragePartition* storage_partition =
      GetStoragePartition(this, site_instance);
  DOMStorageContextWrapper* dom_storage_context =
      static_cast<DOMStorageContextWrapper*>(
          storage_partition->GetDOMStorageContext());

  storage::mojom::SessionStorageControl* session_storage_control =
      dom_storage_context->GetSessionStorageControl();
  if (!session_storage_control)
    return;

  std::string session_namespace_id =
      host->GetDelegate()->GetSessionStorageNamespace(site_instance)->id() +
      "ephemeral-session-storage";
  session_storage_control->DeleteNamespace(session_namespace_id,
                                           false /* should_persist */);

  std::string local_namespace_id =
      host->GetDelegate()->GetSessionStorageNamespace(site_instance)->id() +
      "ephemeral-local-storage";
  session_storage_control->DeleteNamespace(local_namespace_id,
                                           false /* should_persist */);
}

}  // namespace content
