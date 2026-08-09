/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/startup/startup_tab.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/core/browser/container_specifier.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace {

// Switch to specify the container to use for the startup tabs.
constexpr char kContainerSwitch[] = "container";

// Switch to open the startup tabs in a temporary container.
constexpr char kTemporaryContainerSwitch[] = "temporary-container";

// Returns the container to use for the tabs passed via the command line. All
// command line tabs share the same container, matching the "open in new
// temporary container" UI.
//
// `--temporary-container` opens the tabs in a temporary container: a new one,
// or, when `--container` also gives a name, the temporary container with that
// name, created on first use so that a later launch can open more tabs in it.
// `--container` on its own resolves an existing container by name.
//
// Creating a temporary container persists it, so this must not be called more
// than once per launch. Returns an empty specifier (no container) when neither
// switch is given, when the Containers feature is disabled, or when the profile
// has no ContainersService.
containers::ContainerSpecifier MaybeCreateContainerForCommandLineTabs(
    const base::CommandLine& command_line,
    Profile* profile) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return {};
  }

  const std::string container_name =
      command_line.GetSwitchValueUTF8(kContainerSwitch);

  if (command_line.HasSwitch(kTemporaryContainerSwitch)) {
    auto* containers_service = ContainersServiceFactory::GetForProfile(profile);
    if (!containers_service) {
      // The factory selects regular profiles and their off-the-record
      // profiles, so no temporary container is created for guest and system
      // profiles.
      return {};
    }
    auto container =
        container_name.empty()
            ? containers_service->CreateAndPersistTemporaryContainer()
            : containers_service->GetOrCreateTemporaryContainerByName(
                  container_name);
    // Address the temporary container by id: names are not guaranteed to be
    // unique, and a generated one is random.
    return containers::ContainerId(std::move(container->id));
  }

  if (!container_name.empty()) {
    return containers::ContainerName(container_name);
  }

  return {};
}

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
  // Don't create a temporary container when there's nothing to open in it.
  if (!tabs.empty()) {
    const auto container_specifier =
        MaybeCreateContainerForCommandLineTabs(command_line, profile);
    for (auto& tab : tabs) {
      tab.container = container_specifier;
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  return tabs;
}
