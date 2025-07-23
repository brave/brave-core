// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_PARTITIONED_TABS_HANDLER_H_
#define BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_PARTITIONED_TABS_HANDLER_H_

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "base/component_export.h"
#include "brave/components/partitioned_tabs/buildflags/buildflags.h"

#if !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)
namespace content {
class SiteInstance;
class StoragePartitionConfig;
class WebContents;
}  // namespace content
#endif  // BUILDFLAG(PARTITIONED_TABS_READ_ONLY)

namespace partitioned_tabs {

class COMPONENT_EXPORT(PARTITIONED_TABS_BROWSER) PartitionedTabsHandler {
 public:
  // The prefix for the handler id.
  static constexpr char kIdPrefix[] = "partitioned-tabs-";

  virtual ~PartitionedTabsHandler() = default;

  // The id of the partitioned tabs handler.
  virtual const std::string& GetId() const = 0;

#if !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)
  virtual std::optional<content::StoragePartitionConfig>
  MaybeInheritStoragePartition(content::WebContents* web_contents) = 0;

  virtual std::optional<content::StoragePartitionConfig>
  MaybeInheritStoragePartition(
      const std::optional<content::StoragePartitionConfig>&
          storage_partition_config,
      content::SiteInstance* site_instance) = 0;

  virtual bool KeepStoragePartitionInSync(
      content::WebContents* web_contents) = 0;
#endif  // !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)

  virtual bool ShouldRestoreForeignTabFromSync() = 0;
};

}  // namespace partitioned_tabs

#endif  // BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_PARTITIONED_TABS_HANDLER_H_
