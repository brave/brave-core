// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_REGISTRY_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_REGISTRY_H_

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

// Registry for managing contained tab handlers. This singleton class maintains
// a collection of handlers that allow for the isolation of web content in
// separate storage partitions. It also provides methods to generate virtual
// URLs with custom prefixes that have the format
// "handler_id+partition_name:original_url" and exist solely for persisting
// opened tabs in the session service and for synchronization purposes.
//
// The registry ensures forward compatibility: if a handler with a required ID
// is not registered (e.g., in older browser versions), then a webpage won't be
// restored. This allows newer browser versions to introduce new handlers that
// older versions will gracefully ignore.
class COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER) ContainedTabHandlerRegistry {
 public:
  // Returns the singleton instance of the registry.
  static ContainedTabHandlerRegistry& GetInstance();

  // Registers a new contained tab handler. The handler must have a unique ID
  // that starts with the expected prefix. Ownership is transferred to the
  // registry.
  void RegisterContainedTabHandler(
      std::unique_ptr<ContainedTabHandler> handler);

  // Checks if the given storage partition should be inherited based on whether
  // any registered handler matches the partition's domain.
  bool ShouldInheritStoragePartition(
      const content::StoragePartitionConfig& partition_config) const;

  // Determines if the storage partition should be inherited for the given web
  // contents. Returns the partition config if inheritance is needed, nullopt
  // otherwise.
  std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
      content::WebContents* web_contents) const;

  // Determines if the storage partition should be inherited. Uses either the
  // provided storage_partition_config or extracts it from the site_instance.
  // Returns the partition config if inheritance is needed, nullopt otherwise.
  std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
      const std::optional<content::StoragePartitionConfig>&
          storage_partition_config,
      content::SiteInstance* site_instance) const;

  // Generates a virtual URL prefix for the given storage partition key.
  // The prefix has the format "handler_id+partition_name:" and is used
  // to create virtual URLs that exist solely for persisting opened tabs
  // in the session service and for synchronization purposes.
  std::optional<std::string> GetVirtualUrlPrefix(
      const std::pair<std::string, std::string>& storage_partition_key);

  // Attempts to restore the storage partition key from a virtual URL.
  // If successful, populates storage_partition_key with the extracted key,
  // sets url_prefix_length to the length of the virtual prefix, and returns
  // the original URL without the virtual prefix. Returns nullopt if parsing
  // fails. This parsing is used when restoring tabs from session data during
  // synchronization and session restoration.
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
