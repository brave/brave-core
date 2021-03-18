/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_EXTENSION_MANAGEMENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_EXTENSION_MANAGEMENT_H_

#include "chrome/browser/extensions/forced_extensions/install_stage_tracker.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/extension_id.h"
#include "extensions/common/manifest.h"

#define BuildServiceInstanceFor                                           \
  BuildServiceInstanceFor_Unused(content::BrowserContext* context) const; \
  KeyedService* BuildServiceInstanceFor

#include "../../../../../chrome/browser/extensions/extension_management.h"
#undef BuildServiceInstanceFor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_EXTENSION_MANAGEMENT_H_
