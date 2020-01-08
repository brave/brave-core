/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_STP_ENABLED)
#include "brave/browser/content_settings/brave_content_settings_manager_impl.h"
#define ContentSettingsManagerImpl BraveContentSettingsManagerImpl
#endif  // BUILDFLAG(BRAVE_STP_ENABLED)

#include "../../../../chrome/browser/chrome_content_browser_client_receiver_bindings.cc"

#if BUILDFLAG(BRAVE_STP_ENABLED)
#undef ContentSettingsManagerImpl
#endif  // BUILDFLAG(BRAVE_STP_ENABLED)
