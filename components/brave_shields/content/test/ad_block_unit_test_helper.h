// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_AD_BLOCK_UNIT_TEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_AD_BLOCK_UNIT_TEST_HELPER_H_

namespace brave_shields {

class AdBlockService;

// Simulates an empty filter list catalog load, which unblocks the component
// provider gates that guard initial filter set loading. Tests that do not
// install the real catalog component must call this before any filter set
// loading is expected to succeed.
void SetupAdBlockServiceForTesting(AdBlockService* ad_block_service);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_AD_BLOCK_UNIT_TEST_HELPER_H_
