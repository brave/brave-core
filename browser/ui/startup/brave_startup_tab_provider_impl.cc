/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/startup/startup_tab.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/browser/container_specifier.h"
#include "brave/components/containers/core/common/features.h"

namespace {

// Switch to specify the container to use for the startup tabs.
constexpr char kContainerSwitch[] = "container";

}  // namespace
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

StartupTabs BraveStartupTabProviderImpl::GetDistributionFirstRunTabs(
    StartupBrowserCreator* browser_creator) const {
  StartupTabs tabs =
      StartupTabProviderImpl::GetDistributionFirstRunTabs(browser_creator);
  if (first_run::IsChromeFirstRun()) {
    tabs.emplace_back(GURL(kWelcomeURL));
  }
  return tabs;
}

StartupTabs BraveStartupTabProviderImpl::GetCommandLineTabs(
    const base::CommandLine& command_line,
    const base::FilePath& cur_dir,
    Profile* profile) const {
  StartupTabs tabs = StartupTabProviderImpl::GetCommandLineTabs(
      command_line, cur_dir, profile);

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers) &&
      command_line.HasSwitch(kContainerSwitch)) {
    // Get the container name from the command line switch to use for the URLs
    // passed via a command line.
    auto container_specifier = containers::ContainerName(
        command_line.GetSwitchValueUTF8(kContainerSwitch));
    for (auto& tab : tabs) {
      tab.container = container_specifier;
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  return tabs;
}
