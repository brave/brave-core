/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_

#include "third_party/widevine/cdm/buildflags.h"

namespace content {
class WebContents;
}

int GetWidevinePermissionRequestTextFrangmentResourceId();
void RequestWidevinePermission(content::WebContents* web_contents);

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
void InstallBundleOrRestartBrowser();
#endif

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) && !defined(OS_LINUX)
void EnableWidevineCdmComponent(content::WebContents* web_contents);
#endif

void DontAskWidevineInstall(content::WebContents* web_contents, bool dont_ask);

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
