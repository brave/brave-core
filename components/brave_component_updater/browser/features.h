/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace brave_component_updater {

COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER)
BASE_DECLARE_FEATURE(kUseDevUpdaterUrl);

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_FEATURES_H_
