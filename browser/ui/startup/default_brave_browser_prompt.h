/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_STARTUP_DEFAULT_BRAVE_BROWSER_PROMPT_H_
#define BRAVE_BROWSER_UI_STARTUP_DEFAULT_BRAVE_BROWSER_PROMPT_H_

class PrefRegistrySimple;
class Profile;

void RegisterDefaultBraveBrowserPromptPrefs(PrefRegistrySimple* registry);

void ShowDefaultBraveBrowserPrompt(Profile* profile);

void ResetDefaultBraveBrowserPrompt(Profile* profile);

#endif  // BRAVE_BROWSER_UI_STARTUP_DEFAULT_BRAVE_BROWSER_PROMPT_H_
