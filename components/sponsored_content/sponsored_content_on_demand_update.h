/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_ON_DEMAND_UPDATE_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_ON_DEMAND_UPDATE_H_

#include <string>

namespace sponsored_content {

// Asks Component Updater to check for an update to the given Sponsored
// Content component now, at foreground priority, rather than waiting.
void QueueSponsoredContentComponentUpdate(const std::string& component_id);

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_ON_DEMAND_UPDATE_H_
