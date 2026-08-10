/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"

#include "base/command_line.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/startup/startup_tab.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/core/browser/command_line_container.h"
#include "brave/components/containers/core/browser/container_specifier.h"
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
  // Don't create a temporary container when there's nothing to open in it. The
  // factory returns null when the Containers feature is disabled and for guest
  // and system profiles, so those launches get no container either.
  if (auto* containers_service =
          ContainersServiceFactory::GetForProfile(profile);
      containers_service && !tabs.empty()) {
    const containers::ContainerSpecifier container_specifier =
        containers::GetContainerSpecifierForCommandLineTabs(command_line,
                                                            containers_service);
    for (auto& tab : tabs) {
      tab.container = container_specifier;
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  return tabs;
}
