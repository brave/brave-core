/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPEEDREADER_READER_MODE_TOOLBARS_H_
#define BRAVE_BROWSER_UI_SPEEDREADER_READER_MODE_TOOLBARS_H_

class BrowserWindowInterface;

namespace speedreader {

// Shows the reader mode toolbar of each of `browser`'s panes whose tab is
// distilled, and hides the others. Does nothing in windows without views, such
// as a WebUI browser window.
void UpdateReaderModeToolbars(BrowserWindowInterface& browser);

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_SPEEDREADER_READER_MODE_TOOLBARS_H_
