/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_

class PrefRegistrySimple;

// On Android, kWidevineEnabled is written through EnableWidevineCdm() for the
// permission prompt, but r/w through BraveLocalState.java on preference screen
void EnableWidevineCdm();
void DisableWidevineCdm();
int GetWidevinePermissionRequestTextFrangmentResourceId(bool for_restart);
void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry);
bool IsWidevineEnabled();
void SetWidevineEnabled(bool opted_in);

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
