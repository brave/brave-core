/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if defined(OS_IOS)
#include "../../../../../../components/content_settings/core/browser/host_content_settings_map.cc"
#else
#include "brave/components/content_settings/core/browser/brave_content_settings_ephemeral_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#define EphemeralProvider BraveEphemeralProvider
#define PrefProvider BravePrefProvider
#include "../../../../../../components/content_settings/core/browser/host_content_settings_map.cc"
#undef EphemeralProvider
#undef PrefProvider
#endif
