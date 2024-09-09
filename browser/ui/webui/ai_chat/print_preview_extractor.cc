// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/print_preview_extractor.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "brave/services/printing/public/mojom/pdf_to_bitmap_converter.mojom.h"
#include "chrome/browser/pdf/pdf_pref_names.h"
#include "chrome/browser/printing/print_compositor_util.h"
#include "chrome/browser/printing/print_preview_data_service.h"
#include "chrome/browser/printing/print_view_manager_common.h"
#include "chrome/browser/printing/printing_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/services/printing/public/mojom/printing_service.mojom.h"
#include "components/prefs/pref_service.h"
#include "components/printing/browser/print_composite_client.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "pdf/buildflags.h"
#include "printing/print_job_constants.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/skia/include/core/SkBitmap.h"

#if BUILDFLAG(ENABLE_PDF)
#include "base/feature_list.h"
#include "components/pdf/browser/pdf_frame_util.h"
#include "pdf/pdf_features.h"
#endif  // BUILDFLAG(ENABLE_PDF)

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

using printing::PrintCompositeClient;
using printing::mojom::PrintPreviewUI;

namespace ai_chat {

namespace {

// chrome/browser/printing/print_view_manager_common.cc
// Pick the right RenderFrameHost based on the WebContents.
content::RenderFrameHost* GetRenderFrameHostToUse(
    content::WebContents* contents) {
#if BUILDFLAG(ENABLE_PDF)
  // Pick the plugin frame host if `contents` is a PDF viewer guest. If using
  // OOPIF PDF viewer, pick the PDF extension frame host.
  content::RenderFrameHost* full_page_pdf_embedder_host =
      base::FeatureList::IsEnabled(chrome_pdf::features::kPdfOopif)
          ? pdf_frame_util::FindFullPagePdfExtensionHost(contents)
          : printing::GetFullPagePlugin(contents);
  content::RenderFrameHost* pdf_rfh = pdf_frame_util::FindPdfChildFrame(
      full_page_pdf_embedder_host ? full_page_pdf_embedder_host
                                  : contents->GetPrimaryMainFrame());
  if (pdf_rfh) {
    return pdf_rfh;
  }
#endif  // BUILDFLAG(ENABLE_PDF)
  return printing::GetFrameToPrint(contents);
}

}  // namespace

class PreviewPageTextExtractor {
 public:
  PreviewPageTextExtractor(base::ReadOnlySharedMemoryRegion pdf_region,
                           base::OnceCallback<void(std::string)> callback,
                           uint32_t max_page_content_length,
                           std::optional<bool> pdf_use_skia_renderer_enabled)
      : pdf_region_(std::move(pdf_region)),
        callback_(std::move(callback)),
        max_page_content_length_(max_page_content_length) {
    DCHECK(!pdf_to_bitmap_converter_.is_bound());
    GetPrintingService()->BindPdfToBitmapConverter(
        pdf_to_bitmap_converter_.BindNewPipeAndPassReceiver());
    pdf_to_bitmap_converter_.set_disconnect_handler(
        base::BindOnce(&PreviewPageTextExtractor::BitmapConverterDisconnected,
                       base::Unretained(this)));
    if (pdf_use_skia_renderer_enabled.has_value()) {
      pdf_to_bitmap_converter_->SetUseSkiaRendererPolicy(
          pdf_use_skia_renderer_enabled.value());
    }
  }

  void StartExtract() {
    pdf_to_bitmap_converter_->GetPdfPageCount(
        pdf_region_.Duplicate(),
        base::BindOnce(&PreviewPageTextExtractor::OnGetPageCount,
                       base::Unretained(this)));
  }

  void ScheduleNextPageOrComplete() {
    DCHECK_GT(total_page_count_, 0u);
    if (current_page_index_ < total_page_count_) {
      if (current_page_index_) {
        base::StrAppend(&preview_text_, {"\n"});
      }
      pdf_to_bitmap_converter_->GetBitmap(
          pdf_region_.Duplicate(), current_page_index_,
          base::BindOnce(&PreviewPageTextExtractor::OnGetBitmap,
                         base::Unretained(this)));
    } else {
      std::move(callback_).Run(preview_text_);
    }
  }

  void OnGetPageCount(std::optional<uint32_t> page_count) {
    if (!page_count.has_value() || !page_count.value()) {
      std::move(callback_).Run("");
      return;
    }
    total_page_count_ = page_count.value();
    ScheduleNextPageOrComplete();
  }

  void OnGetBitmap(const SkBitmap& bitmap) {
    if (bitmap.drawsNothing()) {
      std::move(callback_).Run(preview_text_);
      return;
    }
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    GetOCRText(bitmap,
               base::BindOnce(&PreviewPageTextExtractor::OnGetTextFromImage,
                              weak_ptr_factory_.GetWeakPtr()));
#else
    std::move(callback_).Run("");
#endif
  }

  void BitmapConverterDisconnected() {
    DLOG(ERROR) << __func__;
    if (callback_) {
      std::move(callback_).Run(preview_text_);
    }
  }

  void OnGetTextFromImage(std::string page_content) {
    VLOG(4) << "Page index(" << current_page_index_
            << ") content: " << page_content;
    base::StrAppend(&preview_text_, {page_content});
    // Stop processing if we have reached the maximum number of pages or the
    // maximum length of the content
    if (current_page_index_ + 1 >= kMaxPreviewPages ||
        preview_text_.length() >= max_page_content_length_) {
      std::move(callback_).Run(preview_text_);
      return;
    }
    ++current_page_index_;
    ScheduleNextPageOrComplete();
  }

 private:
  std::string preview_text_;
  size_t current_page_index_ = 0;
  size_t total_page_count_ = 0;
  base::ReadOnlySharedMemoryRegion pdf_region_;
  base::OnceCallback<void(std::string)> callback_;
  const uint32_t max_page_content_length_;
  mojo::Remote<printing::mojom::PdfToBitmapConverter> pdf_to_bitmap_converter_;
  base::WeakPtrFactory<PreviewPageTextExtractor> weak_ptr_factory_{this};
};

PrintPreviewExtractor::PrintPreviewExtractor(content::WebContents* web_contents,
                                             Profile* profile,
                                             bool is_pdf)
    : web_contents_(web_contents),
      profile_(profile),
      active_chat_tab_helper_(AIChatTabHelper::FromWebContents(web_contents)),
      is_pdf_(is_pdf) {
  DCHECK(web_contents_);
}

PrintPreviewExtractor::~PrintPreviewExtractor() {
  ClearPreviewUIId();
}

mojo::PendingAssociatedRemote<PrintPreviewUI>
PrintPreviewExtractor::BindPrintPreviewUI() {
  return print_preview_ui_receiver_.BindNewEndpointAndPassRemote();
}

void PrintPreviewExtractor::DisconnectPrintPrieviewUI() {
  print_preview_ui_receiver_.reset();
}

bool PrintPreviewExtractor::IsPrintPreviewUIBound() const {
  return print_preview_ui_receiver_.is_bound();
}

void PrintPreviewExtractor::SetPreviewUIId() {
  DCHECK(!print_preview_ui_id_);
  print_preview_ui_id_ =
      printing::PrintPreviewUI::GetPrintPreviewUIIdMap().Add(this);
  printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap()
      [*print_preview_ui_id_] = -1;
}

void PrintPreviewExtractor::ClearPreviewUIId() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!print_preview_ui_id_) {
    return;
  }

  DisconnectPrintPrieviewUI();
  PrintPreviewDataService::GetInstance()->RemoveEntry(*print_preview_ui_id_);
  printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap().erase(
      *print_preview_ui_id_);
  printing::PrintPreviewUI::GetPrintPreviewUIIdMap().Remove(
      *print_preview_ui_id_);
  print_preview_ui_id_.reset();
}

void PrintPreviewExtractor::OnPrintPreviewRequest(int request_id) {
  printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap()
      [*print_preview_ui_id_] = request_id;
}

void PrintPreviewExtractor::SetOptionsFromDocument(
    const printing::mojom::OptionsFromDocumentParamsPtr params,
    int32_t request_id) {}

void PrintPreviewExtractor::DidPrepareDocumentForPreview(
    int32_t document_cookie,
    int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
  if (is_pdf_) {
    return;
  }
  // For case of print preview, page metafile is used to composite into
  // the document PDF at same time.  Need to indicate that this scenario
  // is at play for the compositor.
  auto* client = PrintCompositeClient::FromWebContents(web_contents_);
  DCHECK(client);
  if (client->GetIsDocumentConcurrentlyComposited(document_cookie)) {
    return;
  }

  content::RenderFrameHost* render_frame_host =
      printing::GetFrameToPrint(web_contents_.get());
  // |render_frame_host| could be null when the print preview dialog is closed.
  if (!render_frame_host) {
    return;
  }

  client->PrepareToCompositeDocument(
      document_cookie, render_frame_host, printing::GetCompositorDocumentType(),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&PrintPreviewExtractor::OnPrepareForDocumentToPdfDone,
                         weak_ptr_factory_.GetWeakPtr(), request_id),
          printing::mojom::PrintCompositor::Status::kCompositingFailure));
}
void PrintPreviewExtractor::DidPreviewPage(
    printing::mojom::DidPreviewPageParamsPtr params,
    int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
  uint32_t page_index = params->page_index;
  const printing::mojom::DidPrintContentParams& content = *params->content;
  if (page_index == printing::kInvalidPageIndex ||
      !content.metafile_data_region.IsValid()) {
    return;
  }
  if (is_pdf_) {
    OnCompositePdfPageDone(page_index, params->document_cookie, request_id,
                           printing::mojom::PrintCompositor::Status::kSuccess,
                           content.metafile_data_region.Duplicate());
    return;
  }

  auto* client = PrintCompositeClient::FromWebContents(web_contents_);
  DCHECK(client);

  content::RenderFrameHost* render_frame_host =
      printing::GetFrameToPrint(web_contents_.get());
  if (!render_frame_host) {
    DLOG(ERROR) << "No render frame host for print preview";
    return;
  }

  client->CompositePage(
      params->document_cookie, render_frame_host, content,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&PrintPreviewExtractor::OnCompositePdfPageDone,
                         weak_ptr_factory_.GetWeakPtr(), page_index,
                         params->document_cookie, request_id),
          printing::mojom::PrintCompositor::Status::kCompositingFailure,
          base::ReadOnlySharedMemoryRegion()));
}

void PrintPreviewExtractor::MetafileReadyForPrinting(
    printing::mojom::DidPreviewDocumentParamsPtr params,
    int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
  // pdf content doesn't need to be composited into pdf
  if (is_pdf_) {
    OnCompositeToPdfDone(params->document_cookie, request_id,
                         printing::mojom::PrintCompositor::Status::kSuccess,
                         params->content->metafile_data_region.Duplicate());
    return;
  }
  auto callback = base::BindOnce(&PrintPreviewExtractor::OnCompositeToPdfDone,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 params->document_cookie, request_id);

  // Page metafile is used to composite into the document at same time.
  // Need to provide particulars of how many pages are required before
  // document will be completed.
  auto* client = PrintCompositeClient::FromWebContents(web_contents_);
  client->FinishDocumentComposition(
      params->document_cookie, params->expected_pages_count,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          std::move(callback),
          printing::mojom::PrintCompositor::Status::kCompositingFailure,
          base::ReadOnlySharedMemoryRegion()));
}

void PrintPreviewExtractor::PrintPreviewFailed(int32_t document_cookie,
                                               int32_t request_id) {
  DLOG(ERROR) << __func__ << ": id=" << request_id;
  if (print_preview_ui_id_) {
    printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap()
        [*print_preview_ui_id_] = -1;
  }
}
void PrintPreviewExtractor::PrintPreviewCancelled(int32_t document_cookie,
                                                  int32_t request_id) {
  DLOG(ERROR) << __func__ << ": id=" << request_id;
}
void PrintPreviewExtractor::PrinterSettingsInvalid(int32_t document_cookie,
                                                   int32_t request_id) {
  DLOG(ERROR) << __func__ << ": id=" << request_id;
}

void PrintPreviewExtractor::DidGetDefaultPageLayout(
    printing::mojom::PageSizeMarginsPtr page_layout_in_points,
    const gfx::RectF& printable_area_in_points,
    bool all_pages_have_custom_size,
    bool all_pages_have_custom_orientation,
    int32_t request_id) {}

void PrintPreviewExtractor::DidStartPreview(
    printing::mojom::DidStartPreviewParamsPtr params,
    int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id
           << " , page count: " << params->page_count;
}

void PrintPreviewExtractor::OnPrepareForDocumentToPdfDone(
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status) {
  DVLOG(3) << __func__ << ": id=" << request_id << " , status=" << status;
}

void PrintPreviewExtractor::OnCompositePdfPageDone(
    uint32_t page_index,
    int document_cookie,
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DVLOG(3) << __func__ << ": id=" << request_id << " , status=" << status;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(print_preview_ui_id_);

  if (status == printing::mojom::PrintCompositor::Status::kSuccess) {
    PrintPreviewDataService::GetInstance()->SetDataEntry(
        *print_preview_ui_id_, page_index,
        base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
  }
}

void PrintPreviewExtractor::OnCompositeToPdfDone(
    int document_cookie,
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DVLOG(3) << __func__ << ": id=" << request_id << " , status=" << status;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(print_preview_ui_id_);
  if (status == printing::mojom::PrintCompositor::Status::kSuccess) {
    PrintPreviewDataService::GetInstance()->SetDataEntry(
        *print_preview_ui_id_, printing::COMPLETE_PREVIEW_DOCUMENT_INDEX,
        base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
  }
  OnPreviewReady();
}

void PrintPreviewExtractor::PreviewCleanup() {
  if (!print_preview_ui_id_) {
    return;
  }
  PrintPreviewDataService::GetInstance()->RemoveEntry(*print_preview_ui_id_);
  if (!is_pdf_) {
    print_render_frame_->OnPrintPreviewDialogClosed();
  }
  DisconnectPrintPrieviewUI();
}

void PrintPreviewExtractor::OnPreviewReady() {
  scoped_refptr<base::RefCountedMemory> data;
  CHECK(print_preview_ui_id_);
  PrintPreviewDataService::GetInstance()->GetDataEntry(
      *print_preview_ui_id_, printing::COMPLETE_PREVIEW_DOCUMENT_INDEX, &data);
  if (!data.get()) {
    DLOG(ERROR) << "no data from preview id: " << *print_preview_ui_id_;
    return;
  }
  auto pdf_region = base::ReadOnlySharedMemoryRegion::Create(data->size());
  if (!pdf_region.IsValid()) {
    DLOG(ERROR) << "Failed allocate memory for PDF file";
    return;
  }
  memcpy(pdf_region.mapping.memory(), data->data(), data->size());
  std::optional<bool> pdf_use_skia_renderer_enabled;
  auto* prefs = profile_->GetPrefs();
  if (prefs &&
      prefs->IsManagedPreference(::prefs::kPdfUseSkiaRendererEnabled)) {
    pdf_use_skia_renderer_enabled =
        prefs->GetBoolean(::prefs::kPdfUseSkiaRendererEnabled);
  }
  preview_page_text_extractor_ = std::make_unique<PreviewPageTextExtractor>(
      std::move(pdf_region.region),
      base::BindOnce(&PrintPreviewExtractor::OnGetOCRResult,
                     weak_ptr_factory_.GetWeakPtr()),
      active_chat_tab_helper_->GetMaxPageContentLength(),
      pdf_use_skia_renderer_enabled);
  preview_page_text_extractor_->StartExtract();
}

void PrintPreviewExtractor::OnGetOCRResult(std::string text) {
  active_chat_tab_helper_->OnPreviewTextReady(std::move(text));
  PreviewCleanup();
}

void PrintPreviewExtractor::CreatePrintPreview() {
  if (profile_->GetPrefs()->GetBoolean(::prefs::kPrintPreviewDisabled)) {
    active_chat_tab_helper_->OnPreviewTextReady("");
    return;
  }
  content::RenderFrameHost* rfh = GetRenderFrameHostToUse(web_contents_);
  if (rfh) {
    if (!print_render_frame_.is_bound()) {
      rfh->GetRemoteAssociatedInterfaces()->GetInterface(&print_render_frame_);
    }

    print_render_frame_->SetIsPrintPreviewExtraction(true);
    print_render_frame_->InitiatePrintPreview(false);
    print_render_frame_->SetIsPrintPreviewExtraction(false);

    if (!IsPrintPreviewUIBound()) {
      print_render_frame_->SetPrintPreviewUI(BindPrintPreviewUI());
    }
    if (!print_preview_ui_id_) {
      SetPreviewUIId();
    }
    CHECK(print_preview_ui_id_);

    // A mininum print setting to avoid PrinterSettingsInvalid
    auto settings = base::JSONReader::Read(R"({
   "collate": true,
   "color": 2,
   "copies": 1,
   "deviceName": "Save as PDF",
   "dpiHorizontal": 300,
   "dpiVertical": 300,
   "duplex": 0,
   "headerFooterEnabled": false,
   "isFirstRequest": true,
   "landscape": false,
   "marginsType": 0,
   "mediaSize": {
      "height_microns": 279400,
      "imageable_area_bottom_microns": 0,
      "imageable_area_left_microns": 0,
      "imageable_area_right_microns": 215900,
      "imageable_area_top_microns": 279400,
      "width_microns": 215900
   },
   "pageRange": [  ],
   "pagesPerSheet": 1,
   "printerType": 2,
   "rasterizePDF": false,
   "scaleFactor": 100,
   "scalingType": 0,
   "shouldPrintBackgrounds": false,
   "shouldPrintSelectionOnly": false
  })");
    CHECK(settings);
    auto dict = std::move(*settings).TakeDict();
    dict.Set(printing::kPreviewUIID, print_preview_ui_id_.value());
    dict.Set(printing::kPreviewRequestID, ++preview_request_id_);
    dict.Set(printing::kSettingHeaderFooterTitle, web_contents_->GetTitle());
    dict.Set(printing::kSettingPreviewModifiable, !is_pdf_);
    auto url = web_contents_->GetLastCommittedURL();
    dict.Set(printing::kSettingHeaderFooterURL, url.spec());
    OnPrintPreviewRequest(preview_request_id_);
    print_render_frame_->PrintPreview(std::move(dict));
  }
}

}  // namespace ai_chat
