// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/containers/containers_navigator_utils.h"

#include <optional>

#include "brave/browser/containers/container_specifier_utils.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/container_specifier.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "content/public/browser/security_principal.h"
#include "ui/base/page_transition_types.h"

struct NavigateParams;

namespace containers {

namespace {

// Blank new tabs (Ctrl+T, the new-tab button, "New tab to the right") are
// the only navigations that reach CreateTargetContents with no source
// SiteInstance, a new-tab disposition, a TYPED transition and the NTP URL;
// see chrome::AddAndReturnTabAt. Those honour the Containers "open new tabs
// in" setting. Pages, popups and window.open all carry a source
// SiteInstance and are handled by the caller before this is consulted.
std::optional<content::StoragePartitionConfig>
GetNewTabTargetStoragePartitionConfig(const NavigateParams& params) {
  if (!params.browser ||
      (params.disposition != WindowOpenDisposition::kNewForegroundTab &&
       params.disposition != WindowOpenDisposition::kNewBackgroundTab) ||
      !ui::PageTransitionCoreTypeIs(params.transition,
                                    ui::PAGE_TRANSITION_TYPED) ||
      params.url != chrome::GetNewTabURL(params.browser)) {
    return std::nullopt;
  }

  Profile* profile = params.browser->GetProfile();
  auto* service = ContainersServiceFactory::GetForProfile(profile);
  if (!service || !service->ShouldShowContainerControls()) {
    return std::nullopt;
  }

  // For the temporary case a fresh container is minted here, so this must
  // only run once per tab: CreateTargetContents evaluates it once.
  const auto target = service->GetNewTabTarget();
  ContainerSpecifier specifier;
  if (target && target->is_temporary_container()) {
    specifier = ContainerId(service->CreateAndPersistTemporaryContainer()->id);
  } else if (target && target->is_container_id()) {
    specifier = ContainerId(target->get_container_id());
  }
  return GetStoragePartitionConfigForContainerSpecifier(profile, specifier);
}

}  // namespace

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToInherit(const NavigateParams& params) {
  if (!base::FeatureList::IsEnabled(features::kContainers)) {
    return std::nullopt;
  }

  if (params.storage_partition_config) {
    return containers::MaybeInheritStoragePartition(
        *params.storage_partition_config);
  }

  if (params.source_site_instance) {
    return containers::MaybeInheritStoragePartition(
        params.source_site_instance->GetSecurityPrincipal()
            .GetStoragePartitionConfig());
  }

  return GetNewTabTargetStoragePartitionConfig(params);
}

}  // namespace containers
