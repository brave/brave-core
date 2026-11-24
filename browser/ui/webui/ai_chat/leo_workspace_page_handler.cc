// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_page_handler.h"

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/leo_workspace_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "net/http/http_response_headers.h"
#include "printing/buildflags/buildflags.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include <variant>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/browser/printing/printing_init.h"
#include "components/printing/browser/headless/headless_print_manager.h"
#include "components/printing/browser/print_to_pdf/pdf_print_job.h"
#include "components/printing/browser/print_to_pdf/pdf_print_result.h"
#include "components/printing/browser/print_to_pdf/pdf_print_utils.h"
#include "components/printing/common/print.mojom.h"
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/browser/printing/print_view_manager.h"
#else
#include "chrome/browser/printing/print_view_manager_basic.h"
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)
#endif  // BUILDFLAG(ENABLE_PRINTING)

namespace ai_chat {

namespace {

#if BUILDFLAG(ENABLE_PRINTING)

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
using ActivePrintManager = printing::PrintViewManager;
#else
using ActivePrintManager = printing::PrintViewManagerBasic;
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

// The hidden workspace WebContents is created bare, without the print managers
// a tab gets, so they are attached the first time something is printed from it.
// A workspace that is framed by a tab (e.g. in the file lightbox) already has
// them.
void EnsurePrintingInitialized(content::WebContents* web_contents) {
  if (headless::HeadlessPrintManager::FromWebContents(web_contents) ||
      ActivePrintManager::FromWebContents(web_contents)) {
    return;
  }
  printing::InitializePrintingForWebContents(web_contents);
}

void OnPdfPrinted(mojom::LeoWorkspaceHandler::PrintFileToPdfCallback callback,
                  print_to_pdf::PdfPrintResult result,
                  scoped_refptr<base::RefCountedMemory> data) {
  if (result != print_to_pdf::PdfPrintResult::kPrintSuccess || !data) {
    std::move(callback).Run(
        base::unexpected(print_to_pdf::PdfPrintResultToString(result)));
    return;
  }
  std::move(callback).Run(
      mojo_base::BigBuffer(base::span<const uint8_t>(*data)));
}

#endif  // BUILDFLAG(ENABLE_PRINTING)

// Whether |frame| is an active, live document on |origin|. |origin| must also
// be the frame's URL's origin, so that a frame which inherited its origin (an
// initial about:blank document, or a srcdoc) is not mistaken for one loaded
// from it.
bool IsLiveDocumentOn(content::RenderFrameHost* frame,
                      const url::Origin& origin) {
  return frame->IsActive() && frame->IsRenderFrameLive() &&
         frame->GetLastCommittedOrigin().IsSameOriginWith(origin) &&
         origin.IsSameOriginWith(frame->GetLastCommittedURL());
}

}  // namespace

content::RenderFrameHost* FindLeoWorkspaceFileFrame(
    content::RenderFrameHost* workspace_frame,
    std::string_view viewer_frame_name) {
  if (!workspace_frame || viewer_frame_name.empty()) {
    return nullptr;
  }
  const url::Origin& workspace_origin =
      workspace_frame->GetLastCommittedOrigin();
  if (workspace_origin.scheme() != content::kChromeUIUntrustedScheme ||
      !IsAIChatLeoWorkspaceHost(workspace_origin.host())) {
    return nullptr;
  }
  const url::Origin viewer_origin = url::Origin::Create(GURL(base::StrCat(
      {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator,
       kAIChatLeoWorkspaceViewUIHostPrefix, workspace_origin.host(), "/"})));

  content::RenderFrameHost* viewer_frame = nullptr;
  workspace_frame->ForEachRenderFrameHostWithAction(
      [&](content::RenderFrameHost* frame) {
        if (frame == workspace_frame) {
          return content::RenderFrameHost::FrameIterationAction::kContinue;
        }
        // Only the workspace's own children are candidates.
        if (frame->GetParent() != workspace_frame) {
          return content::RenderFrameHost::FrameIterationAction::kSkipChildren;
        }
        if (frame->GetFrameName() == viewer_frame_name &&
            IsLiveDocumentOn(frame, viewer_origin)) {
          viewer_frame = frame;
          return content::RenderFrameHost::FrameIterationAction::kStop;
        }
        return content::RenderFrameHost::FrameIterationAction::kSkipChildren;
      });
  if (!viewer_frame) {
    return nullptr;
  }

  content::RenderFrameHost* file_frame = nullptr;
  viewer_frame->ForEachRenderFrameHostWithAction(
      [&](content::RenderFrameHost* frame) {
        if (frame == viewer_frame) {
          return content::RenderFrameHost::FrameIterationAction::kContinue;
        }
        if (frame->GetParent() == viewer_frame &&
            IsLiveDocumentOn(frame, viewer_origin) &&
            frame->GetLastCommittedURL().path() != "/") {
          file_frame = frame;
          return content::RenderFrameHost::FrameIterationAction::kStop;
        }
        return content::RenderFrameHost::FrameIterationAction::kSkipChildren;
      });
  return file_frame;
}

LeoWorkspacePageHandler::LeoWorkspacePageHandler(
    content::WebUI* web_ui,
    mojo::PendingReceiver<mojom::LeoWorkspaceHandler> receiver)
    : web_ui_(web_ui), receiver_(this, std::move(receiver)) {}

LeoWorkspacePageHandler::~LeoWorkspacePageHandler() = default;

void LeoWorkspacePageHandler::PrintFileToPdf(
    const std::string& viewer_frame_name,
    bool landscape,
    PrintFileToPdfCallback callback) {
  content::RenderFrameHost* file_frame = FindLeoWorkspaceFileFrame(
      web_ui_->GetRenderFrameHost(), viewer_frame_name);
  if (!file_frame) {
    std::move(callback).Run(
        base::unexpected("The file to print is not being displayed"));
    return;
  }
  // The viewer's service worker answers a file it can't read (e.g. one that
  // doesn't exist) with an error page, which must not be printed as if it were
  // the file.
  const network::mojom::URLResponseHead* response_head =
      file_frame->GetLastResponseHead();
  const int response_code = response_head && response_head->headers
                                ? response_head->headers->response_code()
                                : 0;
  if (!network::IsSuccessfulStatus(response_code)) {
    std::move(callback).Run(base::unexpected(
        base::StrCat({"The file to print could not be loaded (HTTP ",
                      base::NumberToString(response_code), ")"})));
    return;
  }

#if BUILDFLAG(ENABLE_PRINTING)
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(file_frame);
  EnsurePrintingInitialized(web_contents);

  std::variant<printing::mojom::PrintPagesParamsPtr, std::string> params =
      print_to_pdf::GetPrintPagesParams(
          file_frame->GetLastCommittedURL(), landscape,
          /*display_header_footer=*/false,
          // Documents are being converted, not printed on paper, so keep the
          // page's own look.
          /*print_background=*/true, /*scale=*/std::nullopt,
          /*paper_width=*/std::nullopt, /*paper_height=*/std::nullopt,
          /*margin_top=*/std::nullopt, /*margin_bottom=*/std::nullopt,
          /*margin_left=*/std::nullopt, /*margin_right=*/std::nullopt,
          /*header_template=*/std::nullopt, /*footer_template=*/std::nullopt,
          /*prefer_css_page_size=*/true, /*generate_tagged_pdf=*/std::nullopt,
          /*generate_document_outline=*/std::nullopt);
  if (std::holds_alternative<std::string>(params)) {
    std::move(callback).Run(
        base::unexpected(std::move(std::get<std::string>(params))));
    return;
  }
  auto print_pages_params =
      std::move(std::get<printing::mojom::PrintPagesParamsPtr>(params));
  auto on_printed = base::BindOnce(&OnPdfPrinted, std::move(callback));

  if (auto* headless_manager =
          headless::HeadlessPrintManager::FromWebContents(web_contents)) {
    headless_manager->PrintToPdf(file_frame, /*page_ranges=*/"",
                                 std::move(print_pages_params),
                                 std::move(on_printed));
    return;
  }
  auto* print_manager = ActivePrintManager::FromWebContents(web_contents);
  CHECK(print_manager);
  print_manager->PrintToPdf(file_frame, /*page_ranges=*/"",
                            std::move(print_pages_params),
                            std::move(on_printed));
#else
  std::move(callback).Run(
      base::unexpected("Printing is not supported on this platform"));
#endif  // BUILDFLAG(ENABLE_PRINTING)
}

}  // namespace ai_chat
