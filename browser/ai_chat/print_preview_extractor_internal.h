// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_INTERNAL_H_
#define BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_INTERNAL_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/id_map.h"
#include "base/functional/callback.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/print_preview_extractor.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/services/printing/public/mojom/pdf_to_bitmap_converter.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "components/printing/common/print.mojom.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "printing/buildflags/buildflags.h"
#include "printing/mojom/print.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect_f.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace ai_chat {

using ExtractCallback = PrintPreviewExtractor::ExtractCallback;
using CapturePdfCallback = PrintPreviewExtractor::CapturePdfCallback;

class PreviewPageTextExtractor {
 public:
  PreviewPageTextExtractor();
  virtual ~PreviewPageTextExtractor();

  virtual void StartExtract(
      base::ReadOnlySharedMemoryRegion pdf_region,
      PrintPreviewExtractor::Extractor::CallbackVariant callback,
      std::optional<bool> pdf_use_skia_renderer_enabled);

  void BindForTesting(
      mojo::PendingRemote<printing::mojom::PdfToBitmapConverter> converter);

 private:
  void ScheduleNextPageOrComplete();
  void OnGetPageCount(std::optional<uint32_t> page_count);
  void OnGetBitmap(const SkBitmap& bitmap);
  void ProcessNextTextPage(std::string page_content);
  void ProcessNextBitmapPage(const SkBitmap& bitmap);
  void BitmapConverterDisconnected();

  std::string preview_text_;
  size_t current_page_index_ = 0;
  size_t total_page_count_ = 0;
  base::ReadOnlySharedMemoryRegion pdf_region_;
  PrintPreviewExtractor::Extractor::CallbackVariant callback_;
  // raw bytes data of captured pdf pages
  std::vector<std::vector<uint8_t>> pdf_pages_image_data_;
  mojo::Remote<printing::mojom::PdfToBitmapConverter> pdf_to_bitmap_converter_;
  base::WeakPtrFactory<PreviewPageTextExtractor> weak_ptr_factory_{this};
};

class PrintPreviewExtractorInternal : public PrintPreviewExtractor::Extractor,
                                      public printing::mojom::PrintPreviewUI {
 public:
  using GetPrintPreviewUIIdMapCallback =
      base::RepeatingCallback<base::IDMap<printing::mojom::PrintPreviewUI*>&()>;
  using GetPrintPreviewUIRequestIdMapCallback =
      base::RepeatingCallback<base::flat_map<int, int>&()>;
  PrintPreviewExtractorInternal(
      content::WebContents* web_contents,
      Profile* profile,
      bool is_pdf,
      PrintPreviewExtractor::Extractor::CallbackVariant callback,
      GetPrintPreviewUIIdMapCallback id_map_callback,
      GetPrintPreviewUIRequestIdMapCallback request_id_map_callback);

  ~PrintPreviewExtractorInternal() override;

  PrintPreviewExtractorInternal(const PrintPreviewExtractorInternal&) = delete;
  PrintPreviewExtractorInternal& operator=(
      const PrintPreviewExtractorInternal&) = delete;

  void CreatePrintPreview() override;

  std::optional<int32_t> GetPrintPreviewUIIdForTesting() override;

  void SetPreviewPageTextExtractorForTesting(
      std::unique_ptr<PreviewPageTextExtractor> extractor);

  void SendError(const std::string& error);

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

  void DisconnectPrintPreviewUI();

  bool IsPrintPreviewUIBound() const;

  void SetPreviewUIId();

  void ClearPreviewUIId();

  void OnPrintPreviewRequest(int request_id);

  void OnPrepareForDocumentToPdfDone(
      int32_t request_id,
      printing::mojom::PrintCompositor::Status status);

  void OnCompositePdfPageDone(uint32_t page_index,
                              int document_cookie,
                              int32_t request_id,
                              printing::mojom::PrintCompositor::Status status,
                              base::ReadOnlySharedMemoryRegion region);

  void OnCompositeToPdfDone(int document_cookie,
                            int32_t request_id,
                            printing::mojom::PrintCompositor::Status status,
                            base::ReadOnlySharedMemoryRegion region);

  void PreviewCleanup();

  void OnPreviewReady();

  void OnGetOCRResult(base::expected<std::string, std::string> result);

  void OnCaptureBitmapResult(
      base::expected<std::vector<std::vector<uint8_t>>, std::string> result);

 private:
  bool is_pdf_ = false;
  PrintPreviewExtractor::Extractor::CallbackVariant callback_;
  GetPrintPreviewUIIdMapCallback id_map_callback_;
  GetPrintPreviewUIRequestIdMapCallback request_id_map_callback_;
  // unique id to avoid conflicts with other print preview UIs
  std::optional<int32_t> print_preview_ui_id_;
  mojo::AssociatedReceiver<PrintPreviewUI> print_preview_ui_receiver_{this};

  int preview_request_id_ = -1;
  std::unique_ptr<PreviewPageTextExtractor> preview_page_text_extractor_;
  mojo::AssociatedRemote<printing::mojom::PrintRenderFrame> print_render_frame_;

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;

  base::WeakPtrFactory<PrintPreviewExtractorInternal> weak_ptr_factory_{this};
};
}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_INTERNAL_H_
