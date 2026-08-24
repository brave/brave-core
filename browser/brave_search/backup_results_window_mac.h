// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_WINDOW_MAC_H_
#define BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_WINDOW_MAC_H_

#include <memory>

namespace content {
class WebContents;
}  // namespace content

namespace gfx {
class Rect;
class Size;
}  // namespace gfx

namespace brave_search {

// Manages a hidden NSWindow that contains a backup results
// WebContents view, to resemble a real tab.
class BackupResultsWindowMac {
 public:
  static std::unique_ptr<BackupResultsWindowMac> Create(
      content::WebContents* web_contents,
      const gfx::Rect& window_bounds,
      const gfx::Size& view_size);

  BackupResultsWindowMac(const BackupResultsWindowMac&) = delete;
  BackupResultsWindowMac& operator=(const BackupResultsWindowMac&) = delete;

  virtual ~BackupResultsWindowMac();

 protected:
  BackupResultsWindowMac();
};

}  // namespace brave_search

#endif  // BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_WINDOW_MAC_H_
