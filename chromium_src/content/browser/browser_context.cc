/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/browser_context.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/session_storage_namespace_impl.h"
#include "content/browser/renderer_host/navigation_controller_impl.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/site_instance_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {

mojo::PendingRemote<storage::mojom::BlobStorageContext>
GetRemoteBlobStorageContextFor(BrowserContext* browser_context) {
  return content::ChromeBlobStorageContext::GetRemoteFor(browser_context);
}

scoped_refptr<content::SessionStorageNamespace> CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id,
    absl::optional<std::string> clone_from_namespace_id) {
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
  SiteInstanceImpl* site_instance_impl =
      static_cast<SiteInstanceImpl*>(web_contents->GetSiteInstance());
  DCHECK(site_instance_impl);

  return static_cast<NavigationControllerImpl&>(web_contents->GetController())
      .GetSessionStorageNamespace(
          site_instance_impl->GetStoragePartitionConfig())
      ->id();
}

}  // namespace content

namespace content {
bool BrowserContext::IsTor() const {
  return false;
}
}  // namespace content

#include "src/content/browser/browser_context.cc"

#include "content/browser/tld_ephemeral_lifetime.cc"
