// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/pdf_text_extractor.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/ai_chat/content/browser/pdf_text_helper.h"
#include "components/pdf/browser/pdf_document_helper.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

PdfTextExtractor::PdfTextExtractor() = default;

PdfTextExtractor::~PdfTextExtractor() = default;

void PdfTextExtractor::ExtractText(content::BrowserContext* browser_context,
                                   const base::FilePath& pdf_path,
                                   ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  LoadInWebContents(browser_context, pdf_path);
}

void PdfTextExtractor::ExtractText(content::BrowserContext* browser_context,
                                   std::vector<uint8_t> pdf_bytes,
                                   ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  WriteTempFileAndLoad(browser_context, std::move(pdf_bytes),
                       FILE_PATH_LITERAL("pdf"));
}

network::mojom::WebSandboxFlags PdfTextExtractor::AdditionalUnsandboxFlags()
    const {
  // Plugins are required for the PDF viewer MimeHandlerView.
  return network::mojom::WebSandboxFlags::kPlugins;
}

void PdfTextExtractor::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  TryRegisterForDocumentLoad();
}

void PdfTextExtractor::TryRegisterForDocumentLoad() {
  if (registered_for_load_ || !GetWebContents()) {
    return;
  }
  auto* pdf_helper =
      pdf::PDFDocumentHelper::MaybeGetForWebContents(GetWebContents());
  if (!pdf_helper) {
    return;
  }
  registered_for_load_ = true;
  pdf_helper->RegisterForDocumentLoadComplete(
      base::BindOnce(&PdfTextExtractor::OnDocumentLoadComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PdfTextExtractor::OnDocumentLoadComplete() {
  OnDocumentReady();
}

void PdfTextExtractor::OnDocumentReady() {
  ExtractTextFromLoadedPdf(GetWebContents(),
                           base::BindOnce(&PdfTextExtractor::Finish,
                                          weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace ai_chat
