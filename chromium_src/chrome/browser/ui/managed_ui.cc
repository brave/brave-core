/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_service_factory.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "chrome/browser/enterprise/util/managed_browser_utils.h"

#define ShouldDisplayManagedUi ShouldDisplayManagedUi_ChromiumImpl
#include <chrome/browser/ui/managed_ui.cc>
#undef ShouldDisplayManagedUi

namespace {

bool IsManagedOnlyByBraveOrigin(Profile* profile) {
  brave_origin::BraveOriginService* brave_origin_service =
      brave_origin::BraveOriginServiceFactory::GetForProfile(profile);

  if (!brave_origin_service || !brave_origin_service->IsBraveOriginUser()) {
    return false;
  }

  // If the browser is currently managed but was not managed before BraveOrigin
  // policies were applied, then we know the management is solely due to
  // BraveOrigin policies
  return enterprise_util::IsBrowserManaged(profile) &&
         !brave_origin_service->WasManagedBeforeBraveOrigin();
}

}  // namespace

// Our override implementation
bool ShouldDisplayManagedUi(Profile* profile) {
  // If managed only by BraveOrigin, don't show the management UI
  if (IsManagedOnlyByBraveOrigin(profile)) {
    return false;
  }

  // Otherwise, use the original logic
  return ShouldDisplayManagedUi_ChromiumImpl(profile);
}
