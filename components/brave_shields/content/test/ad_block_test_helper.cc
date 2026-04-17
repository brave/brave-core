// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/test/ad_block_test_helper.h"

#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

namespace brave_shields {

void SetupAdBlockServiceForTesting(AdBlockService* ad_block_service) {
  ad_block_service->component_service_manager()->SetFilterListCatalog({});
}

}  // namespace brave_shields
