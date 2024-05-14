// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_H_

#include <optional>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "components/printing/common/print.mojom.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace content {
class WebContents;
}  // namespace content

class Profile;

namespace ai_chat {
class AIChatTabHelper;
class PreviewPageTextExtractor;

class PrintPreviewExtractor : public printing::mojom::PrintPreviewUI {
 public:
  PrintPreviewExtractor(content::WebContents* web_contents,
                        Profile* profile,
                        bool is_pdf);
  ~PrintPreviewExtractor() override;

  PrintPreviewExtractor(const PrintPreviewExtractor&) = delete;
  PrintPreviewExtractor& operator=(const PrintPreviewExtractor&) = delete;

  void CreatePrintPreview();

 private:
  // printing::mojom::PrintPreviewUI:
  void SetOptionsFromDocument(
      const printing::mojom::OptionsFromDocumentParamsPtr params,
      int32_t request_id) override;
  void DidPrepareDocumentForPreview(int32_t document_cookie,
                                    int32_t request_id) override;
  void DidPreviewPage(printing::mojom::DidPreviewPageParamsPtr params,
                      int32_t request_id) override;
  void MetafileReadyForPrinting(
      printing::mojom::DidPreviewDocumentParamsPtr params,
      int32_t request_id) override;
  void PrintPreviewFailed(int32_t document_cookie, int32_t request_id) override;
  void PrintPreviewCancelled(int32_t document_cookie,
                             int32_t request_id) override;
  void PrinterSettingsInvalid(int32_t document_cookie,
                              int32_t request_id) override;
  void DidGetDefaultPageLayout(
      printing::mojom::PageSizeMarginsPtr page_layout_in_points,
      const gfx::RectF& printable_area_in_points,
      bool all_pages_have_custom_size,
      bool all_pages_have_custom_orientation,
      int32_t request_id) override;
  void DidStartPreview(printing::mojom::DidStartPreviewParamsPtr params,
                       int32_t request_id) override;

  mojo::PendingAssociatedRemote<PrintPreviewUI> BindPrintPreviewUI();
  void DisconnectPrintPrieviewUI();
  bool IsPrintPreviewUIBound() const;
  void SetPreviewUIId();
  void ClearPreviewUIId();
  void OnPrintPreviewRequest(int request_id);

  void OnPreviewReady();
  void PreviewCleanup();
  void OnGetOCRResult(std::string text);

  void OnPrepareForDocumentToPdfDone(
      int32_t request_id,
      printing::mojom::PrintCompositor::Status status);
  void OnCompositePdfPageDone(uint32_t page_index,
                              int32_t document_cookie,
                              int32_t request_id,
                              printing::mojom::PrintCompositor::Status status,
                              base::ReadOnlySharedMemoryRegion region);
  void OnCompositeToPdfDone(int document_cookie,
                            int32_t request_id,
                            printing::mojom::PrintCompositor::Status status,
                            base::ReadOnlySharedMemoryRegion region);

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<AIChatTabHelper> active_chat_tab_helper_ = nullptr;
  bool is_pdf_ = false;
  // unique id to avoid conflicts with other print preview UIs
  std::optional<int32_t> print_preview_ui_id_;
  mojo::AssociatedReceiver<PrintPreviewUI> print_preview_ui_receiver_{this};

  int preview_request_id_ = -1;
  std::unique_ptr<PreviewPageTextExtractor> preview_page_text_extractor_;
  mojo::AssociatedRemote<printing::mojom::PrintRenderFrame> print_render_frame_;

  base::WeakPtrFactory<PrintPreviewExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_H_
