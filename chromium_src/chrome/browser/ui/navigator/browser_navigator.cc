/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
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
#include "content/public/browser/security_principal.h"
#include "ui/base/page_transition_types.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace {

void UpdateBraveScheme(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

void MaybeOverridePopupDisposition(NavigateParams* params) {
  if (base::FeatureList::IsEnabled(features::kForcePopupToBeOpenedAsTab) &&
      params->disposition == WindowOpenDisposition::NEW_POPUP) {
    params->disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  }
}

void UpdateParams(NavigateParams* params) {
  UpdateBraveScheme(params);
  MaybeOverridePopupDisposition(params);
}

#if BUILDFLAG(ENABLE_CONTAINERS)
// Blank new tabs (Ctrl+T, the new-tab button, "New tab to the right") are
// the only navigations that reach CreateTargetContents with no source
// SiteInstance, a new-tab disposition, a TYPED transition and the NTP URL;
// see chrome::AddAndReturnTabAt. Those honour the Containers "open new tabs
// in" setting. Pages, popups and window.open all carry a source
// SiteInstance and are handled by the caller before this is consulted.
std::optional<content::StoragePartitionConfig>
GetNewTabDefaultStoragePartitionConfig(const NavigateParams& params) {
  if (!params.browser ||
      (params.disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB &&
       params.disposition != WindowOpenDisposition::NEW_BACKGROUND_TAB) ||
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
  const auto new_tab_default = service->GetNewTabDefault();
  containers::ContainerSpecifier specifier;
  if (new_tab_default->temporary_container) {
    specifier = containers::ContainerId(
        service->CreateAndPersistTemporaryContainer()->id);
  } else if (!new_tab_default->container_id.empty()) {
    specifier = containers::ContainerId(new_tab_default->container_id);
  }
  return containers::GetStoragePartitionConfigForContainerSpecifier(profile,
                                                                    specifier);
}

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToInherit(const NavigateParams& params) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
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

  return GetNewTabDefaultStoragePartitionConfig(params);
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

}  // namespace

#include <chrome/browser/ui/navigator/browser_navigator.cc>
