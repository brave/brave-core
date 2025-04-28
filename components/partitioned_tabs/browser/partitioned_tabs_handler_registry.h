// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_PARTITIONED_TABS_HANDLER_REGISTRY_H_
#define BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_PARTITIONED_TABS_HANDLER_REGISTRY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/no_destructor.h"
#include "brave/components/partitioned_tabs/browser/partitioned_tabs_handler.h"
#include "brave/components/partitioned_tabs/buildflags/buildflags.h"

namespace partitioned_tabs {

class COMPONENT_EXPORT(PARTITIONED_TABS_BROWSER)
    PartitionedTabsHandlerRegistry {
 public:
  static PartitionedTabsHandlerRegistry& GetInstance();

  void RegisterPartitionedTabsHandler(
      std::unique_ptr<PartitionedTabsHandler> handler);

// IsolatedTabsHandler:
#if !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)
  std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
      content::WebContents* web_contents);

  std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
      const std::optional<content::StoragePartitionConfig>&
          storage_partition_config,
      content::SiteInstance* site_instance);

  bool KeepStoragePartitionInSync(content::WebContents* web_contents);
#endif  // !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)

  bool ShouldRestoreForeignTabFromSync(
      const std::map<std::string, std::string>& extended_info_map);

 private:
  friend class base::NoDestructor<PartitionedTabsHandlerRegistry>;

  PartitionedTabsHandlerRegistry();
  ~PartitionedTabsHandlerRegistry();

  std::vector<std::unique_ptr<PartitionedTabsHandler>> handlers_;
};

}  // namespace partitioned_tabs

#endif  // BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_PARTITIONED_TABS_HANDLER_REGISTRY_H_
