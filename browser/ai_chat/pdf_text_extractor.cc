// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/pdf_text_extractor.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/content/browser/pdf_text_helper.h"
#include "components/pdf/browser/pdf_document_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

namespace {

constexpr base::TimeDelta kExtractionTimeout = base::Seconds(30);

std::optional<base::FilePath> WritePdfToTempFile(
    std::vector<uint8_t> pdf_bytes) {
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    return std::nullopt;
  }
  base::FilePath temp_path;
  if (!base::CreateTemporaryFileInDir(temp_dir, &temp_path)) {
    return std::nullopt;
  }
  // Rename to add .pdf extension so MIME type detection works
  base::FilePath pdf_path = temp_path.AddExtension(FILE_PATH_LITERAL("pdf"));
  if (!base::Move(temp_path, pdf_path)) {
    base::DeleteFile(temp_path);
    return std::nullopt;
  }
  if (!base::WriteFile(pdf_path, pdf_bytes)) {
    base::DeleteFile(pdf_path);
    return std::nullopt;
  }
  return pdf_path;
}

void DeleteTempFile(const base::FilePath& path) {
  if (!path.empty()) {
    base::DeleteFile(path);
  }
}

}  // namespace

PdfTextExtractor::PdfTextExtractor() = default;

PdfTextExtractor::~PdfTextExtractor() {
  Cleanup();
}

void PdfTextExtractor::ExtractText(content::BrowserContext* browser_context,
                                   const base::FilePath& pdf_path,
                                   ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  LoadPdfInWebContents(browser_context, pdf_path);
}

void PdfTextExtractor::ExtractText(content::BrowserContext* browser_context,
                                   std::vector<uint8_t> pdf_bytes,
                                   ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&WritePdfToTempFile, std::move(pdf_bytes)),
      base::BindOnce(&PdfTextExtractor::OnTempFileWritten,
                     weak_ptr_factory_.GetWeakPtr(), browser_context));
}

// content::WebContentsDelegate:

bool PdfTextExtractor::ShouldSuppressDialogs(content::WebContents* source) {
  return true;
}

void PdfTextExtractor::CanDownload(const GURL& url,
                                   const std::string& request_method,
                                   base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(false);
}

bool PdfTextExtractor::IsWebContentsCreationOverridden(
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url) {
  return true;
}

bool PdfTextExtractor::CanEnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame) {
  return false;
}

bool PdfTextExtractor::CanDragEnter(
    content::WebContents* source,
    const content::DropData& data,
    blink::DragOperationsMask operations_allowed) {
  return false;
}

void PdfTextExtractor::RequestKeyboardLock(content::WebContents* web_contents,
                                           bool esc_key_locked) {
  web_contents->GotResponseToKeyboardLockRequest(false);
}

// content::WebContentsObserver:

void PdfTextExtractor::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  TryRegisterForDocumentLoad();
}

void PdfTextExtractor::PrimaryMainFrameRenderProcessGone(
    base::TerminationStatus status) {
  DVLOG(1) << "PdfTextExtractor: Renderer process gone";
  Finish(std::nullopt);
}

void PdfTextExtractor::OnTempFileWritten(
    content::BrowserContext* browser_context,
    std::optional<base::FilePath> temp_path) {
  if (!temp_path || !callback_) {
    Finish(std::nullopt);
    return;
  }
  temp_file_path_ = *temp_path;
  LoadPdfInWebContents(browser_context, temp_file_path_);
}

void PdfTextExtractor::LoadPdfInWebContents(
    content::BrowserContext* browser_context,
    const base::FilePath& pdf_path) {
  content::WebContents::CreateParams create_params(browser_context);
  create_params.is_never_composited = true;
  // Allow scripts (JS/WASM), origin (Mojo bridge), navigation (subframes),
  // and plugins (required for PDF viewer MimeHandlerView).
  create_params.starting_sandbox_flags =
      network::mojom::WebSandboxFlags::kAll &
      ~network::mojom::WebSandboxFlags::kScripts &
      ~network::mojom::WebSandboxFlags::kOrigin &
      ~network::mojom::WebSandboxFlags::kNavigation &
      ~network::mojom::WebSandboxFlags::kPlugins;
  web_contents_ = content::WebContents::Create(create_params);
  web_contents_->SetOwnerLocationForDebug(FROM_HERE);
  web_contents_->SetDelegate(this);
  Observe(web_contents_.get());

  timeout_timer_.Start(
      FROM_HERE, kExtractionTimeout,
      base::BindOnce(&PdfTextExtractor::OnTimeout, base::Unretained(this)));

  GURL file_url = net::FilePathToFileURL(pdf_path);
  web_contents_->GetController().LoadURL(file_url, content::Referrer(),
                                         ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                         std::string());
}

void PdfTextExtractor::TryRegisterForDocumentLoad() {
  if (registered_for_load_ || !web_contents_) {
    return;
  }
  auto* pdf_helper =
      pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents_.get());
  if (!pdf_helper) {
    return;
  }
  registered_for_load_ = true;
  pdf_helper->RegisterForDocumentLoadComplete(
      base::BindOnce(&PdfTextExtractor::OnDocumentLoadComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PdfTextExtractor::OnDocumentLoadComplete() {
  ExtractTextFromLoadedPdf(web_contents_.get(),
                           base::BindOnce(&PdfTextExtractor::Finish,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void PdfTextExtractor::OnTimeout() {
  DVLOG(1) << "PdfTextExtractor: Extraction timed out";
  Finish(std::nullopt);
}

void PdfTextExtractor::Finish(std::optional<std::string> result) {
  DVLOG(1) << "PdfTextExtractor: Finish has_result=" << result.has_value();
  timeout_timer_.Stop();
  Cleanup();
  if (callback_) {
    std::move(callback_).Run(std::move(result));
  }
}

void PdfTextExtractor::Cleanup() {
  if (web_contents_) {
    Observe(nullptr);
    web_contents_->SetDelegate(nullptr);
    web_contents_.reset();
  }
  if (!temp_file_path_.empty()) {
    base::ThreadPool::PostTask(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&DeleteTempFile, std::move(temp_file_path_)));
    temp_file_path_.clear();
  }
}

}  // namespace ai_chat
