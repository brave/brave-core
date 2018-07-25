/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_PAGES_H_
#define BRAVE_BROWSER_UI_BRAVE_PAGES_H_

class Browser;

namespace brave {

void ShowBraveAdblock(Browser* browser);
void ShowBraveRewards(Browser* browser);
void ShowBraveSync(Browser* browser);
void LoadBraveSyncJsLib(Browser* browser);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_PAGES_H_
