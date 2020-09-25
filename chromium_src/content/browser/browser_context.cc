/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/browser_context.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "brave/content/browser/ephemeral_storage_partition.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/session_storage_namespace_impl.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "../../../../content/browser/browser_context.cc"

namespace content {

namespace {

using EphemeralStoragePartitionMap =
    std::map<std::string, std::unique_ptr<EphemeralStoragePartition>>;

EphemeralStoragePartitionMap& ephemeral_storage_partition_map() {
  static base::NoDestructor<EphemeralStoragePartitionMap>
      ephemeral_storage_partition_map;
  return *ephemeral_storage_partition_map.get();
}

}  // namespace

scoped_refptr<content::SessionStorageNamespace> CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id) {
  content::DOMStorageContextWrapper* context_wrapper =
      static_cast<content::DOMStorageContextWrapper*>(
          partition->GetDOMStorageContext());

  return content::SessionStorageNamespaceImpl::Create(context_wrapper,
                                                      namespace_id);
}

void BrowserContext::DeleteInMemoryStoragePartitionForMainFrameURL(
    const GURL& main_frame_url) {
  std::string storage_domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          main_frame_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  ephemeral_storage_partition_map().erase(storage_domain);
}

EphemeralStoragePartition*
BrowserContext::GetEphemeralStoragePartitionForMainFrameURL(
    SiteInstance* site_instance,
    const GURL& main_frame_url) {
  std::string storage_domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          main_frame_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  auto& map = ephemeral_storage_partition_map();
  auto existing_partition = map.find(storage_domain);
  if (existing_partition == map.end()) {
    auto* normal_partition =
        static_cast<StoragePartitionImpl*>(BrowserContext::GetStoragePartition(
            this, site_instance, /* can_create = */ true));
    auto new_ephemeral_partition = std::make_unique<EphemeralStoragePartition>(
        this, normal_partition,
        base::FilePath(std::string("ephemeral") + storage_domain));

    EphemeralStoragePartition* partition_ptr = new_ephemeral_partition.get();
    map[storage_domain] = std::move(new_ephemeral_partition);
    return partition_ptr;
  }

  return existing_partition->second.get();
}

}  // namespace content
