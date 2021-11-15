/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_REGISTRY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_REGISTRY_H_

// Pull in all includes from content_settings_registry.h because Init is too
// common a name.
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

#define Init                                                              \
  Init();                                                                 \
  void RegisterBraveContentSettingsTypes(const ContentSettingsType& type, \
                                         const std::string& name);        \
  void BraveInit

#include "src/components/content_settings/core/browser/content_settings_registry.h"
#undef Init

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_REGISTRY_H_
