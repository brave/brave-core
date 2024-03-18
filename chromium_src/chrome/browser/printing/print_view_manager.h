/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_H_

namespace printing {
class PrintViewManager;
using PrintViewManager_BraveImpl = PrintViewManager;
}  // namespace printing

#define PrintViewManager PrintViewManager_ChromiumImpl
#define PrintPreviewForWebNode       \
  PrintPreviewForWebNodeUnused();    \
  friend PrintViewManager_BraveImpl; \
  void PrintPreviewForWebNode
#include "src/chrome/browser/printing/print_view_manager.h"  // IWYU pragma: export
#undef PrintPreviewForWebNode
#undef PrintViewManager

namespace printing {

class PrintViewManager : public PrintViewManager_ChromiumImpl {
 public:
  explicit PrintViewManager(content::WebContents* web_contents);

  PrintViewManager(const PrintViewManager&) = delete;
  PrintViewManager& operator=(const PrintViewManager&) = delete;

  ~PrintViewManager() override;

  static void CreateForWebContents(content::WebContents* web_contents);

  static PrintViewManager* FromWebContents(content::WebContents* web_contents);

 private:
  void RejectPrintPreviewRequestIfRestricted(
      content::GlobalRenderFrameHostId rfh_id,
      base::OnceCallback<void(bool should_proceed)> callback) override;
};

}  // namespace printing

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_H_
