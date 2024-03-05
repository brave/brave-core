/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_H_

#define PrintViewManager PrintViewManager_ChromiumImpl
#define PrintPreviewForWebNode        \
  PrintPreviewForWebNodeUnused();     \
  friend class BravePrintViewManager; \
  void PrintPreviewForWebNode
#include "src/chrome/browser/printing/print_view_manager.h"  // IWYU pragma: export
#undef PrintPreviewForWebNode
#undef PrintViewManager

namespace printing {

class BravePrintViewManager : public PrintViewManager_ChromiumImpl {
 public:
  explicit BravePrintViewManager(content::WebContents* web_contents);

  BravePrintViewManager(const BravePrintViewManager&) = delete;
  BravePrintViewManager& operator=(const BravePrintViewManager&) = delete;

  ~BravePrintViewManager() override;

  static void CreateForWebContents(content::WebContents* web_contents);

  static BravePrintViewManager* FromWebContents(
      content::WebContents* web_contents);

  static void BindPrintManagerHost(
      mojo::PendingAssociatedReceiver<mojom::PrintManagerHost> receiver,
      content::RenderFrameHost* rfh);

  bool PrintForSystemDialogNow(base::OnceClosure dialog_shown_callback);

  bool BasicPrint(content::RenderFrameHost* rfh);

  bool PrintPreviewNow(content::RenderFrameHost* rfh, bool has_selection);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  bool PrintPreviewWithPrintRenderer(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedRemote<mojom::PrintRenderer> print_renderer);
#endif

  void PrintPreviewForNodeUnderContextMenu(content::RenderFrameHost* rfh);

  void PrintPreviewAlmostDone();

  void PrintPreviewDone();

  content::RenderFrameHost* print_preview_rfh();

  static void SetReceiverImplForTesting(PrintManager* impl);

 private:
  void RejectPrintPreviewRequestIfRestricted(
      content::GlobalRenderFrameHostId rfh_id,
      base::OnceCallback<void(bool should_proceed)> callback) override;
};

// This main purpose of this is for adding `friend class PrintViewManager` to
// upstream PrintViewManager to avoid
// resursive macro expansion that leads to
// `friend class PrintViewManager_ChromiumImpl`
typedef BravePrintViewManager PrintViewManager;

}  // namespace printing

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_H_
