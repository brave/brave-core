/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_IMPORTED_BROWSER_WINDOW_H_
#define BRAVE_COMMON_IMPORTER_IMPORTED_BROWSER_WINDOW_H_

#include <string>
#include <vector>
#include "url/gurl.h"

struct ImportedBrowserTab {
  ImportedBrowserTab();
  ImportedBrowserTab(const ImportedBrowserTab& other);
  ~ImportedBrowserTab();

  int key;
  GURL location;
};

struct ImportedBrowserWindow {
  ImportedBrowserWindow();
  ImportedBrowserWindow(const ImportedBrowserWindow& other);
  ~ImportedBrowserWindow();

  int top;
  int left;
  int width;
  int height;

  bool focused;

  // "normal", "minimized", "maximized", or "fullscreen"
  std::string state;

  // Warning: activeFrameKey may not reference an existing key in the frames
  // Array. For example, if a private or private w/ Tor tab was focused when the
  // browser closed, it will be the activeFrameKey, but it will not be
  // persisted.
  int activeFrameKey;

  std::vector<ImportedBrowserTab> tabs;
};

struct ImportedWindowState {
  ImportedWindowState();
  ImportedWindowState(const ImportedWindowState& other);
  ~ImportedWindowState();

  std::vector<ImportedBrowserWindow> windows;
  std::vector<ImportedBrowserTab> pinnedTabs;
};

#endif  // BRAVE_COMMON_IMPORTER_IMPORTED_BROWSER_WINDOW_H_
