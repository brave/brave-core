/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_POLICY_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_POLICY_PROVIDER_H_

#include "components/prefs/pref_change_registrar.h"

#define PrefChangeRegistrar         \
  friend class BravePolicyProvider; \
  PrefChangeRegistrar
#define ReadManagedContentSettings \
  brave_dummy() {}                 \
  virtual void ReadManagedContentSettings
#include "src/components/content_settings/core/browser/content_settings_policy_provider.h"
#undef PrefChangeRegistrar
#undef ReadManagedContentSettings

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_POLICY_PROVIDER_H_
