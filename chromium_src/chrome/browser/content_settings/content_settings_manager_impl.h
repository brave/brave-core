/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_MANAGER_IMPL_H_

#include "brave/components/brave_shields/browser/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_STP_ENABLED)
#define CONTENT_SETTINGS_MANAGER_IMPL_H_ \
  friend class BraveContentSettingsManagerImpl;
#else
#define CONTENT_SETTINGS_MANAGER_IMPL_H_
#endif

#include "../../../../../chrome/browser/content_settings/content_settings_manager_impl.h"
#undef CONTENT_SETTINGS_MANAGER_IMPL_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_MANAGER_IMPL_H_
