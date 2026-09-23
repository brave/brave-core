/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/web_applications/web_app_launch_process.h"

#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "components/services/app_service/public/cpp/app_launch_params.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/browser/containers/container_specifier_utils.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/core/browser/command_line_container.h"
#include "brave/components/containers/core/browser/container_specifier.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace web_app {

namespace {

// Navigates a command line PWA launch, first resolving --container /
// --temporary-container into the storage partition its WebContents should use,
// reusing the same service calls as the normal command line tab path. Without
// the containers feature (build- or runtime-disabled) or a container, this is a
// plain Navigate(&nav_params).
content::WebContents* NavigateWebAppWithContainerPartition(
    [[maybe_unused]] Profile* profile,
    [[maybe_unused]] const apps::AppLaunchParams& launch_params,
    NavigateParams& nav_params) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers) &&
      !nav_params.storage_partition_config) {
    if (auto* containers_service =
            ContainersServiceFactory::GetForProfile(profile)) {
      const containers::ContainerSpecifier container_specifier =
          containers::GetContainerSpecifierForCommandLineTabs(
              launch_params.command_line, containers_service);
      nav_params.storage_partition_config =
          containers::GetStoragePartitionConfigForContainerSpecifier(
              profile, container_specifier);
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  Navigate(&nav_params);
  return nav_params.navigated_or_inserted_contents;
}

}  // namespace

}  // namespace web_app

#include <chrome/browser/ui/web_applications/web_app_launch_process.cc>
