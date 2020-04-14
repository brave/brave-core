/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
#define BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_

class Browser;

namespace brave {

void NewOffTheRecordWindowTor(Browser*);
void NewTorConnectionForSite(Browser*);
void AddNewProfile();
void OpenGuestProfile();
void ToggleSpeedreader(Browser* browser);

}  // namespace brave


#endif  // BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
