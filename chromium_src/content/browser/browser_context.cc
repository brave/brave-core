/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/browser_context.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/session_storage_namespace_impl.h"
#include "content/browser/renderer_host/navigation_controller_impl.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"

namespace content {

scoped_refptr<content::SessionStorageNamespace> CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id,
    base::Optional<std::string> clone_from_namespace_id) {
  content::DOMStorageContextWrapper* context_wrapper =
      static_cast<content::DOMStorageContextWrapper*>(
          partition->GetDOMStorageContext());

  if (clone_from_namespace_id) {
    return content::SessionStorageNamespaceImpl::CloneFrom(
        context_wrapper, namespace_id, clone_from_namespace_id.value(), true);
  } else {
    return content::SessionStorageNamespaceImpl::Create(context_wrapper,
                                                        namespace_id);
  }
}

std::string GetSessionStorageNamespaceId(WebContents* web_contents) {
  RenderViewHostImpl* rvh_impl =
      static_cast<RenderViewHostImpl*>(web_contents->GetRenderViewHost());
  DCHECK(rvh_impl);

  SiteInstanceImpl* site_instance_impl =
      static_cast<SiteInstanceImpl*>(web_contents->GetSiteInstance());
  DCHECK(site_instance_impl);

  return rvh_impl->frame_tree()
      ->controller()
      .GetSessionStorageNamespace(site_instance_impl->GetSiteInfo())
      ->id();
}

}  // namespace content

namespace content {
bool BrowserContext::IsTor() const {
  return false;
}
}  // namespace content

#include "../../../../content/browser/browser_context.cc"
#include "content/browser/tld_ephemeral_lifetime.cc"
