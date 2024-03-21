/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/printing/print_view_manager.h"

#define PrintViewManager PrintViewManager_ChromiumImpl
#include "src/chrome/browser/printing/print_view_manager.cc"
#undef PrintViewManager

namespace printing {

PrintViewManager::PrintViewManager(content::WebContents* web_contents)
    : PrintViewManager_ChromiumImpl(web_contents) {}

PrintViewManager::~PrintViewManager() = default;

// static
void PrintViewManager::CreateForWebContents(
    content::WebContents* web_contents) {
  if (!FromWebContents(web_contents)) {
    web_contents->SetUserData(PrintViewManager_ChromiumImpl::UserDataKey(),
                              std::make_unique<PrintViewManager>(web_contents));
  }
}

// static
PrintViewManager* PrintViewManager::FromWebContents(
    content::WebContents* web_contents) {
  return static_cast<PrintViewManager*>(
      web_contents->GetUserData(PrintViewManager_ChromiumImpl::UserDataKey()));
}

// static
void PrintViewManager::BindPrintManagerHost(
    mojo::PendingAssociatedReceiver<mojom::PrintManagerHost> receiver,
    content::RenderFrameHost* rfh) {
  PrintViewManager_ChromiumImpl::BindPrintManagerHost(std::move(receiver), rfh);
}

bool PrintViewManager::PrintForSystemDialogNow(
    base::OnceClosure dialog_shown_callback) {
  return PrintViewManager_ChromiumImpl::PrintForSystemDialogNow(
      std::move(dialog_shown_callback));
}

bool PrintViewManager::BasicPrint(content::RenderFrameHost* rfh) {
  return PrintViewManager_ChromiumImpl::BasicPrint(rfh);
}

bool PrintViewManager::PrintPreviewNow(content::RenderFrameHost* rfh,
                                       bool has_selection) {
  return PrintViewManager_ChromiumImpl::PrintPreviewNow(rfh, has_selection);
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
bool PrintViewManager::PrintPreviewWithPrintRenderer(
    content::RenderFrameHost* rfh,
    mojo::PendingAssociatedRemote<mojom::PrintRenderer> print_renderer) {
  return PrintViewManager_ChromiumImpl::PrintPreviewWithPrintRenderer(
      rfh, std::move(print_renderer));
}
#endif

void PrintViewManager::PrintPreviewForNodeUnderContextMenu(
    content::RenderFrameHost* rfh) {
  PrintViewManager_ChromiumImpl::PrintPreviewForNodeUnderContextMenu(rfh);
}

void PrintViewManager::PrintPreviewAlmostDone() {
  PrintViewManager_ChromiumImpl::PrintPreviewAlmostDone();
}

void PrintViewManager::PrintPreviewDone() {
  PrintViewManager_ChromiumImpl::PrintPreviewDone();
}

content::RenderFrameHost* PrintViewManager::print_preview_rfh() {
  return PrintViewManager_ChromiumImpl::print_preview_rfh();
}

// static
void PrintViewManager::SetReceiverImplForTesting(PrintManager* impl) {
  PrintViewManager_ChromiumImpl::SetReceiverImplForTesting(impl);
}

void PrintViewManager::RejectPrintPreviewRequestIfRestricted(
    content::GlobalRenderFrameHostId rfh_id,
    base::OnceCallback<void(bool should_proceed)> callback) {
  // Initiated from AIChatUI
  if (print_preview_state_ == NOT_PREVIEWING && !g_receiver_for_testing) {
    std::move(callback).Run(/*should_proceed=*/false);
    return;
  }
  PrintViewManager_ChromiumImpl::RejectPrintPreviewRequestIfRestricted(
      rfh_id, std::move(callback));
}

}  // namespace printing
