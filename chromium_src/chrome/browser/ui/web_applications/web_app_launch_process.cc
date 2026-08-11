/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/web_applications/web_app_launch_process.h"

#include "base/command_line.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/web_applications/web_app_launch_utils.h"
#include "components/services/app_service/public/cpp/app_launch_params.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_specifier_utils.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/core/browser/command_line_container.h"
#include "brave/components/containers/core/browser/container_specifier.h"

namespace web_app {

namespace {

// Resolves the container for a command line PWA launch (--container /
// --temporary-container) into the storage partition its WebContents should use,
// reusing the same service calls as the normal command line tab path, then
// navigates. Without a container this is a plain NavigateWebAppUsingParams().
content::WebContents* NavigateWebAppInContainerUsingParams(
    Profile* profile,
    const apps::AppLaunchParams& launch_params,
    NavigateParams& nav_params) {
  if (!nav_params.storage_partition_config) {
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
  return NavigateWebAppUsingParams(nav_params);
}

}  // namespace

}  // namespace web_app
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/web_applications/web_app_launch_process.cc>
