/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_WIDEVINE_BUNDLE_UTIL_H_
#define BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_WIDEVINE_BUNDLE_UTIL_H_

#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
void RegisterWidevineCdmToCdmRegistry();
#endif

#endif  // BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_WIDEVINE_BUNDLE_UTIL_H_
