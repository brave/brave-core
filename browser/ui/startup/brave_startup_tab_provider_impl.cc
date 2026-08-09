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
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/core/browser/container_specifier.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace {

// Switch to specify the container to use for the startup tabs.
constexpr char kContainerSwitch[] = "container";

// Switch to open the startup tabs in a newly created temporary container.
constexpr char kTemporaryContainerSwitch[] = "temporary-container";

// Returns the container to use for the tabs passed via the command line. All
// command line tabs share the same container, matching the "open in new
// temporary container" UI.
//
// With `--temporary-container` this creates and persists a new container, so it
// must not be called more than once per launch, and it returns an empty
// specifier when the profile has no ContainersService. Returns an empty
// specifier (no container) when neither switch is given or the Containers
// feature is disabled.
containers::ContainerSpecifier MaybeCreateContainerForCommandLineTabs(
    const base::CommandLine& command_line,
    Profile* profile) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return {};
  }

  const bool has_temporary_switch =
      command_line.HasSwitch(kTemporaryContainerSwitch);
  const bool has_container_switch = command_line.HasSwitch(kContainerSwitch);
  if (has_temporary_switch && has_container_switch) {
    // A temporary container is the more isolated of the two, so prefer it.
    DVLOG(1) << "--" << kTemporaryContainerSwitch << " overrides --"
             << kContainerSwitch;
  }

  if (has_temporary_switch) {
    auto* containers_service = ContainersServiceFactory::GetForProfile(profile);
    if (!containers_service) {
      // The factory selects regular profiles and their off-the-record
      // profiles, so no temporary container is created for guest and system
      // profiles.
      return {};
    }
    // Address the temporary container by id: its name is randomly generated and
    // is not guaranteed to be unique.
    auto container = containers_service->CreateAndPersistTemporaryContainer();
    return containers::ContainerId(std::move(container->id));
  }

  if (has_container_switch) {
    return containers::ContainerName(
        command_line.GetSwitchValueUTF8(kContainerSwitch));
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
