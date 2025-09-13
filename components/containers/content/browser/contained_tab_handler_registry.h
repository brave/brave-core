// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_REGISTRY_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_REGISTRY_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/no_destructor.h"
#include "brave/components/containers/content/browser/contained_tab_handler.h"
#include "url/gurl.h"

namespace content {
class SiteInstance;
class StoragePartitionConfig;
class WebContents;
}  // namespace content

namespace containers {

class COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER) ContainedTabHandlerRegistry {
 public:
  static ContainedTabHandlerRegistry& GetInstance();

  void RegisterContainedTabHandler(
      std::unique_ptr<ContainedTabHandler> handler);

  std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
      content::WebContents* web_contents);

  std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
      const std::optional<content::StoragePartitionConfig>&
          storage_partition_config,
      content::SiteInstance* site_instance);

  std::optional<std::string> GetVirtualUrlPrefix(
      const std::pair<std::string, std::string>& storage_partition_key);

  std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
      const GURL& url,
      std::pair<std::string, std::string>& storage_partition_key,
      size_t& url_prefix_length);

 private:
  friend class base::NoDestructor<ContainedTabHandlerRegistry>;

  ContainedTabHandlerRegistry();
  ~ContainedTabHandlerRegistry();

  std::vector<std::unique_ptr<ContainedTabHandler>> handlers_;
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_REGISTRY_H_
