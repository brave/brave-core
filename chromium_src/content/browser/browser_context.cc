/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/browser_context.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/session_storage_namespace_impl.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "../../../../content/browser/browser_context.cc"

namespace content {

scoped_refptr<content::SessionStorageNamespace> CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id) {
  content::DOMStorageContextWrapper* context_wrapper =
      static_cast<content::DOMStorageContextWrapper*>(
          partition->GetDOMStorageContext());

  return content::SessionStorageNamespaceImpl::Create(context_wrapper,
                                                      namespace_id);
}

std::string GetSessionStorageNamespaceId(WebContents* web_contents) {
  return web_contents->GetRenderViewHost()
      ->GetDelegate()
      ->GetSessionStorageNamespace(web_contents->GetSiteInstance())
      ->id();
}

std::string URLToEphemeralStorageDomain(const GURL& url) {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  // GetDomainAndRegistry might return an empty string if this host is an IP
  // address or a file URL.
  if (domain.empty())
    domain = url::Origin::Create(url.GetOrigin()).Serialize();

  return domain;
}

scoped_refptr<EphemeralStoragePartition>
BrowserContext::GetOrCreateEphemeralStoragePartition(
    std::string storage_domain) {
  return EphemeralStoragePartition::GetOrCreate(this, storage_domain);
}

EphemeralStoragePartition* BrowserContext::GetExistingEphemeralStoragePartition(
    const GURL& url) {
  return EphemeralStoragePartition::Get(this, URLToEphemeralStorageDomain(url));
}

}  // namespace content
