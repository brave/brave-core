// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_PDF_TEXT_EXTRACTOR_H_
#define BRAVE_BROWSER_AI_CHAT_PDF_TEXT_EXTRACTOR_H_

#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_chat/file_text_extractor_base.h"
#include "services/network/public/cpp/web_sandbox_flags.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace ai_chat {

// Extracts text from a PDF by loading it in a hidden background WebContents.
// The PDF viewer extension + ScreenAI OCR pipeline runs, then page text is
// extracted via PDFDocumentHelper::GetPageText().
//
// The extractor should be kept alive until the callback fires.
class PdfTextExtractor : public FileTextExtractorBase {
 public:
  PdfTextExtractor();
  ~PdfTextExtractor() override;

 private:
  // FileTextExtractorBase:
  void OnDocumentReady() override;
  network::mojom::WebSandboxFlags AdditionalUnsandboxFlags() const override;

  // content::WebContentsObserver:
  // PDF uses DidFinishLoad → TryRegisterForDocumentLoad instead of the
  // base class's DocumentOnLoadCompletedInPrimaryMainFrame.
  void DocumentOnLoadCompletedInPrimaryMainFrame() override {}
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  void TryRegisterForDocumentLoad();
  void OnDocumentLoadComplete();

  bool registered_for_load_ = false;
  base::WeakPtrFactory<PdfTextExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_PDF_TEXT_EXTRACTOR_H_
